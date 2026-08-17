/* Header */
#include "worker_flash.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Defines */
#define SERIAL_SELF_ROLE                        PORT_FLASHING
#define FLASH_PAGE_SIZE							256
#define FLASH_SENSOR_DISCONNECT_TIMEOUT_MS		5000
#define FLASH_ACQUIRE_TIMEOUT_MS				2000
#define FLASH_BOOTLOADER_WAIT_MS                2000
#define FLASH_PAGE_WRITE_RETRIES                3
#define DEVICE_SIGNATURE_LENGTH                 3
#define STK_RETRIE_TIMEOUT_MS                   500
#define STK_RETRIE_NUM                          5
#define STK_MESSAGE_START                       0x1B
#define STK_TOKEN                               0x0E
#define STK_CMD_SIGN_ON                         0x01
#define STK_CMD_LOAD_ADDRESS                    0x06
#define STK_CMD_READ_SIGNATURE_ISP              0x1B  
#define STK_CMD_PROGRAM_FLASH_ISP               0x13
#define STK_CMD_LEAVE_PROGMODE_ISP              0x11
#define STK_CMD_READ_FLASH_ISP                  0x14
#define STK_STATUS_CMD_OK                       0x00
#define STK_STATUS_CMD_TOUT                     0x80
#define STK_STATUS_RDY_BSY_TOUT                 0x81
#define STK_STATUS_CMD_FAILED                   0xC0
#define STK_STATUS_CKSUM_ERROR                  0xC1
#define STK_STATUS_CMD_UNKNOWN                  0xC9
#define STK_MESSAGE_START_SIZE  		        1
#define STK_SEQ_SIZE                            1   
#define STK_SIZE_SIZE                           2
#define STK_MAX_BODY_SIZE                       275
#define STK_CHECKSUM_SIZE                       1
#define STK_TOKEN_SIZE                          1
#define STK_FRAME_SIZE_MAX                      (STK_MESSAGE_START_SIZE + STK_SEQ_SIZE + STK_SIZE_SIZE + STK_TOKEN_SIZE + STK_MAX_BODY_SIZE + STK_CHECKSUM_SIZE)
#define HEX_MINIMUM_LINE_LENGTH                 11
#define HEX_MAXIMUM_LINE_DATA_LENGTH            255

/* Type definitions */
typedef enum
{
    WAIT_START, 
    WAIT_SEQ, 
    WAIT_SIZE_H, 
    WAIT_SIZE_L, 
    WAIT_TOKEN, 
    WAIT_BODY, 
    WAIT_CHECKSUM
} stk500_rx_state_t;

typedef struct flash_page_t
{
    uint8_t data[FLASH_PAGE_SIZE];
    size_t count;              
    uint32_t address;          
    struct flash_page_t* next;
} flash_page_t;

typedef struct
{
    flash_page_t* first_page;
    flash_page_t* last_page;
    size_t page_count;
} flash_page_list_t;

typedef struct
{
    uint8_t seq;
    uint16_t body_len;
    uint8_t body[STK_MAX_BODY_SIZE];
} stk500_response_t;

/* Static local variables */
static volatile LONG should_stop = 0;
static HANDLE thread_handle = NULL;
static uint32_t page_read_write_num = 0;
static uint32_t pages_read_write_total = 0;

/* Static function definitions */
static void flash_set_progress(bridge_flash_state_t state, uint8_t percent)
{
    bridge_flash_progress_t progress = { .state = state, .progress_percentage = percent };
    bridge_publish_flash_progress(&progress);
    return;
}

static void free_page_list(flash_page_list_t* list)
{
    /* Check if list is valid */
    if (list == NULL) return;

    /* Free all pages */
    flash_page_t* current = list->first_page;
    while (current != NULL)
    {
        flash_page_t* next = current->next;
        free(current);
        current = next;
    }

    /* Clear list */
    list->first_page = NULL;
    list->last_page = NULL;
    list->page_count = 0;
    return;
}

static bool stk500_send_command(uint8_t seq, const uint8_t* body, uint16_t body_len)
{
    /* Check arguments */
	if (body == NULL) return false;
    if (!body_len || (body_len > STK_MAX_BODY_SIZE)) return false;

    /* Buffer for frame */
    uint8_t frame[STK_FRAME_SIZE_MAX];
    size_t offset = 0;

    /* Write header */
    frame[offset++] = STK_MESSAGE_START;
    frame[offset++] = seq;
    frame[offset++] = (uint8_t)((body_len >> 8) & 0xFF);
    frame[offset++] = (uint8_t)(body_len & 0xFF);
    frame[offset++] = STK_TOKEN;

    /* Copy body */
    memcpy(&frame[offset], body, body_len);
    offset += body_len;

    /* Calculate checksum & write it to buffer */
    uint8_t checksum = 0;
    for (size_t byte_index = 0; byte_index < offset; byte_index++) checksum ^= frame[byte_index];
    frame[offset++] = checksum;

    /* Write frame to serial port */
    size_t written = serial_port_write(SERIAL_SELF_ROLE, frame, offset);
    return written == offset;
}

static bool stk500_receive_response(stk500_response_t* out, uint32_t timeout_ms)
{
    /* Check arguments */
    if (out == NULL) return false;

    /* Receive buffer & state */
    uint8_t raw[STK_FRAME_SIZE_MAX];
    size_t collected = 0;
    uint16_t body_len = 0;
    stk500_rx_state_t state = WAIT_START;

    /* Get starting time */
    uint64_t start_ms = GetTickCount64();

    while (true)
    {
        /* Check timeout */
        uint64_t elapsed = GetTickCount64() - start_ms;
        if (elapsed >= timeout_ms) return false;
        uint32_t remaining = (uint32_t)(timeout_ms - elapsed);

        /* Get byte from serial port */
        uint8_t byte;
        size_t got = serial_port_read(SERIAL_SELF_ROLE, &byte, 1, remaining);
        if (got == 0) return false;

        switch (state)
        {
        case WAIT_START:
            if (byte == STK_MESSAGE_START)
            {
                raw[0] = byte;
                collected = 1;
                state = WAIT_SEQ;
            }
            break;

        case WAIT_SEQ:
            raw[1] = byte;
            collected = 2;
            state = WAIT_SIZE_H;
            break;

        case WAIT_SIZE_H:
            raw[2] = byte;
            collected = 3;
            state = WAIT_SIZE_L;
            break;

        case WAIT_SIZE_L:
            raw[3] = byte;
            collected = 4;
            body_len = ((uint16_t)raw[2] << 8) | raw[3];
            if (body_len > STK_MAX_BODY_SIZE)
            {
                state = WAIT_START;
                collected = 0;
                break;
            }
            state = WAIT_TOKEN;
            break;

        case WAIT_TOKEN:
            if (byte != STK_TOKEN)
            {
                state = WAIT_START;
                collected = 0;
                break;
            }
            raw[4] = byte;
            collected = 5;
            state = (body_len == 0) ? WAIT_CHECKSUM : WAIT_BODY;
            break;

        case WAIT_BODY:
            raw[collected++] = byte;
            if (collected == (size_t)(5 + body_len)) state = WAIT_CHECKSUM;
            break;

        case WAIT_CHECKSUM:
            raw[collected++] = byte;
            {
                uint8_t checksum = 0;
                for (size_t byte_index = 0; byte_index < collected - 1; byte_index++) checksum ^= raw[byte_index];
                if (checksum != raw[collected - 1])
                {
                    state = WAIT_START;
                    collected = 0;
                    break;
                }
            }
            out->seq = raw[1];
            out->body_len = body_len;
            memcpy(out->body, &raw[5], body_len);
            return true;
        }
    }
}

static bool stk500_send_and_receive(uint8_t seq, const uint8_t* body, uint16_t body_len, stk500_response_t* out, uint32_t timeout_ms)
{
    if (!stk500_send_command(seq, body, body_len)) return false;
    if (!stk500_receive_response(out, timeout_ms)) return false;
    if (out->seq != seq) return false;
    return true;
}

static bool stk500_ping(uint8_t* seq)
{
    for (size_t attempt = 0; attempt < STK_RETRIE_NUM; attempt++)
    {
        uint8_t body = STK_CMD_SIGN_ON;
        stk500_response_t resp;
        bool ok = stk500_send_and_receive(*seq, &body, sizeof(body), &resp, STK_RETRIE_TIMEOUT_MS);
        (*seq)++;

        if (ok && (resp.body_len >= 2) && (resp.body[0] == STK_CMD_SIGN_ON) && (resp.body[1] == STK_STATUS_CMD_OK))
        {
            bridge_log_queue_push("CMD_SIGN_ON, Success");
            return true;
        }
        else
        {
            bridge_log_queue_push("CMD_SIGN_ON, Failed! attempt again...");
        }
        Sleep(300);
    }
    return false;
}

static bool stk500_read_signature(uint8_t* seq, uint8_t out_signature[DEVICE_SIGNATURE_LENGTH])
{
    char log_buffer[128];
    for (size_t index = 0; index < DEVICE_SIGNATURE_LENGTH; index++)
    {
        uint8_t body[6] = { STK_CMD_READ_SIGNATURE_ISP, 3, 0x30, 0x00, (uint8_t)index, 0x00 };
        stk500_response_t resp;
        bool ok = stk500_send_and_receive(*seq, body, sizeof(body), &resp, 500);
        (*seq)++;

        if (!ok) return false;
        if (resp.body_len < 4) return false;
        if (resp.body[0] != STK_CMD_READ_SIGNATURE_ISP) return false;
        if (resp.body[1] != STK_STATUS_CMD_OK) return false;

        out_signature[index] = resp.body[2];
        snprintf(log_buffer, sizeof(log_buffer), "Received signature byte %zu: %02X", index, out_signature[index]);
        bridge_log_queue_push(log_buffer);
    }
    return true;
}

static bool enter_bootloader_and_check_signature(uint16_t port_number, int32_t baud_rate, uint8_t* seq)
{
    /* Check arguments */
    if ((port_number < COM_PORT_NUM_MIN) || (port_number > COM_PORT_NUM_MAX)) return false;
    if ((baud_rate < COM_PORT_BAUD_MIN) || (baud_rate > COM_PORT_BAUD_MAX)) return false;   
    if (seq == NULL) return false;  

    /* Get port name */
    char port_name[16];
    snprintf(port_name, sizeof(port_name), "COM%u", port_number);

    /* Open serial port */
    if (!serial_port_open(SERIAL_SELF_ROLE, port_name, (uint32_t)baud_rate)) return false;
    bridge_publish_connection_info(true, port_number, baud_rate, false);

    /* Wait for bootloader to settle */
    serial_port_flush(SERIAL_SELF_ROLE);

    /* Ping */
    if (!stk500_ping(seq))
    {
        return false;
    }
    else
    {
        bridge_log_queue_push("CMD_SIGN_ON, Bootloader answered");
    }

    /* Get signature */
    uint8_t signature[3];
    if (!stk500_read_signature(seq, signature))
    {
        bridge_log_queue_push("Signature read failed");
        return false;
    }

    if ((signature[0] != 0x1E) || (signature[1] != 0x98) || (signature[2] != 0x01))
    {
        char msg[96];
        snprintf(msg, sizeof(msg), "Wrong signature: %02X %02X %02X (expected 1E 98 01)", signature[0], signature[1], signature[2]);
        bridge_log_queue_push(msg);
        return false;
    }

    bridge_log_queue_push("Signature correct (ATmega2560)");
    return true;
}

static bool stk500_leave_progmode(uint8_t* seq)
{
    uint8_t body[3] = { STK_CMD_LEAVE_PROGMODE_ISP, 1 /* preDelay ms */, 1 /* postDelay ms */ };
    stk500_response_t resp;
    bool ok = stk500_send_and_receive(*seq, body, sizeof(body), &resp, 1000);
    (*seq)++;

    return ok && (resp.body_len >= 2) && (resp.body[0] == STK_CMD_LEAVE_PROGMODE_ISP) && (resp.body[1] == STK_STATUS_CMD_OK);
}

static bool write_page(uint8_t* seq, uint32_t word_address, const uint8_t* data, size_t count)
{
    /* Check arguments */
    if (seq == NULL) return false;
    if ((data == NULL) || !count) return false;

    /* CMD_LOAD_ADDRESS */
    uint8_t load_body[] = 
    {
        STK_CMD_LOAD_ADDRESS,
        (uint8_t)((word_address >> 24) & 0xFF),
        (uint8_t)((word_address >> 16) & 0xFF),
        (uint8_t)((word_address >> 8) & 0xFF),
        (uint8_t)(word_address & 0xFF)
    };
    stk500_response_t load_resp;
    if (!stk500_send_and_receive(*seq, load_body, sizeof(load_body), &load_resp, 1000)) return false;
    (*seq)++;
    if ((load_resp.body_len < 2) || (load_resp.body[0] != STK_CMD_LOAD_ADDRESS) || (load_resp.body[1] != STK_STATUS_CMD_OK)) return false;

    /* CMD_PROGRAM_FLASH_ISP */
    uint8_t body[STK_MAX_BODY_SIZE];
    body[0] = STK_CMD_PROGRAM_FLASH_ISP;
    body[1] = (uint8_t)((count >> 8) & 0xFF);
    body[2] = (uint8_t)(count & 0xFF);
    body[3] = 0x91;   /* KORRIGIERT: Page-Mode + Timed-Delay + Write-Page */
    body[4] = 10;     /* delay in ms - Praxiswert laut Empfehlung */
    body[5] = 0x40;   /* Load Page Low Byte */
    body[6] = 0x4C;   /* Write Program Memory Page */
    body[7] = 0x20;   /* Read Program Memory (fuer Value-Polling ungenutzt bei Timed-Delay, trotzdem mitschicken) */
    body[8] = 0x00;   /* Poll1 unused */
    body[9] = 0x00;   /* Poll2 unused */
    memcpy(&body[10], data, count);

    stk500_response_t prog_resp;
    if (!stk500_send_and_receive(*seq, body, (uint16_t)(10 + count), &prog_resp, 5000)) return false;   /* 5s laut Doku fuer PROGRAM_FLASH */
    (*seq)++;

    return (prog_resp.body_len >= 2) && (prog_resp.body[0] == STK_CMD_PROGRAM_FLASH_ISP) && (prog_resp.body[1] == STK_STATUS_CMD_OK);
}

static bool read_page(uint8_t* seq, uint32_t word_address, uint8_t* out_data, size_t count)
{
    if (seq == NULL) return false;
    if ((out_data == NULL) || !count || (count > FLASH_PAGE_SIZE)) return false;

    uint8_t load_body[5] =
    {
        STK_CMD_LOAD_ADDRESS,
        (uint8_t)((word_address >> 24) & 0xFF),
        (uint8_t)((word_address >> 16) & 0xFF),
        (uint8_t)((word_address >> 8) & 0xFF),
        (uint8_t)(word_address & 0xFF)
    };
    stk500_response_t load_resp;
    if (!stk500_send_and_receive(*seq, load_body, sizeof(load_body), &load_resp, 1000)) return false;
    (*seq)++;
    if ((load_resp.body_len < 2) || (load_resp.body[0] != STK_CMD_LOAD_ADDRESS) || (load_resp.body[1] != STK_STATUS_CMD_OK)) return false;

    uint8_t body[4] = { STK_CMD_READ_FLASH_ISP, (uint8_t)((count >> 8) & 0xFF), (uint8_t)(count & 0xFF), 0x20 };
    stk500_response_t read_resp;
    if (!stk500_send_and_receive(*seq, body, sizeof(body), &read_resp, 5000)) return false;
    (*seq)++;

    if (read_resp.body_len != (uint16_t)(2 + count + 1)) return false;
    if (read_resp.body[0] != STK_CMD_READ_FLASH_ISP) return false;
    if (read_resp.body[1] != STK_STATUS_CMD_OK) return false;
    if (read_resp.body[2 + count] != STK_STATUS_CMD_OK) return false;

    memcpy(out_data, &read_resp.body[2], count);
    return true;
}

static bool write_pages(const flash_page_list_t* pages, uint8_t* seq)
{
    if (pages == NULL) return false;

    char msg[128];
    size_t index = 0;
    for (flash_page_t* page = pages->first_page; page != NULL; page = page->next, index++)
    {
        uint32_t word_address = page->address / 2;   /* STK500v2 erwartet Wort-, keine Byte-Adressen */

        bool ok = false;
        for (int retry = 0; (retry < FLASH_PAGE_WRITE_RETRIES) && !ok; retry++)
        {
            ok = write_page(seq, word_address, page->data, page->count);
            if (!ok) Sleep(100);
        }

        if (!ok)
        {
            snprintf(msg, sizeof(msg), "Writing of page %zu failed", index);
            bridge_log_queue_push(msg);
            return false;
        }
        else
        {
            snprintf(msg, sizeof(msg), "Writing page %zu/%zu successfully", index + 1, pages->page_count);
            bridge_log_queue_push(msg);
        }

        page_read_write_num++;
        flash_set_progress(BRIDGE_FLASH_WRITING_FIRMWARE, (uint8_t)((page_read_write_num * 100) / pages_read_write_total));
    }
    return true;
}

static bool verify_pages(const flash_page_list_t* pages, uint8_t* seq)
{
    if (pages == NULL) return false;

    size_t index = 0;
    char msg[64];
    for (flash_page_t* page = pages->first_page; page != NULL; page = page->next, index++)
    {
        uint32_t word_address = page->address / 2;
        uint8_t read_back[FLASH_PAGE_SIZE];

        bool ok = false;
        for (int retry = 0; (retry < FLASH_PAGE_WRITE_RETRIES) && !ok; retry++)
        {
            ok = read_page(seq, word_address, read_back, page->count);
            if (!ok) Sleep(100);
        }

        if (!ok)
        {
            
            snprintf(msg, sizeof(msg), "Reading of page %zu failed", index);
            bridge_log_queue_push(msg);
            return false;
        }

        if (memcmp(read_back, page->data, page->count) != 0)
        {
            snprintf(msg, sizeof(msg), "Verification of page %zu failed", index);
            bridge_log_queue_push(msg);
            return false;
        }

		snprintf(msg, sizeof(msg), "Verification of page %zu/%zu successful", index + 1, pages->page_count);   
        bridge_log_queue_push(msg);

        page_read_write_num++;
        flash_set_progress(BRIDGE_FLASH_VERIFYING_FIRMWARE, (uint8_t)((page_read_write_num * 100) / pages_read_write_total));
    }
    return true;
}

static int hex_char_to_value(char c)
{
    if ((c >= '0') && (c <= '9')) return c - '0';
    if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
    if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
    return -1;
}

static bool hex_byte_to_value(const char* text, uint8_t* out)
{
    /* Check arguments */
    if (text == NULL) return false;
    if (out == NULL) return false;

    /* Convert hex characters to value */
    int high = hex_char_to_value(text[0]);
    int low = hex_char_to_value(text[1]);
    
    /* Check if conversion was successful */
    if ((high < 0) || (low < 0)) return false;

    /* Combine the high and low nibbles */
    *out = (uint8_t)((high << 4) | low);
    return true;
}

static flash_page_t* get_or_create_page(flash_page_list_t* list, flash_page_t** cache, uint32_t page_address)
{
    /* Check arguments */
	if (list == NULL) return NULL;
	if (cache == NULL) return NULL;

    /* Check if the requested page is already in the cache */
    if ((*cache != NULL) && ((*cache)->address == page_address)) return *cache;

    /* Search for the page in the list */
    for (flash_page_t* page = list->first_page; page != NULL; page = page->next)
    {
        if (page->address == page_address)
        {
            *cache = page;
            return page;
        }
    }

    /* Create a new page */
    flash_page_t* new_page = (flash_page_t*)malloc(sizeof(flash_page_t));
    if (new_page == NULL) return NULL;

    /* Fill page with 0xFF */
    memset(new_page->data, 0xFF, sizeof(new_page->data));   
    new_page->count = FLASH_PAGE_SIZE; 
    new_page->address = page_address;
    new_page->next = NULL;

    /* Append new page to the list */
    if (list->last_page == NULL)
    {
        list->first_page = new_page;
    }
    else
    {
        list->last_page->next = new_page;
    }
    list->last_page = new_page;
    list->page_count++;

    *cache = new_page;
    return new_page;
}

static bool parse_hex_file(const char* filename, flash_page_list_t* out_pages)
{
    /* Check arguments & initialize */
    if ((filename == NULL) || (out_pages == NULL)) return false;
    memset(out_pages, 0, sizeof(*out_pages));

    /* Get path of the executable & delete executable name from path */
    char exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if ((len == 0) || (len == sizeof(exe_path))) return false;
    char* last_slash = strrchr(exe_path, '\\');
    if (last_slash != NULL) *(last_slash + 1) = '\0';

    /* Add firmware path */
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%sresources\\firmware\\%s", exe_path, filename);

    /* Open firmware file */
    char msg[512];
    FILE* file = fopen(full_path, "r");
    if (file == NULL)
    {
        snprintf(msg, sizeof(msg), "Firmware file %s not found in path %s", filename, full_path);
        bridge_log_queue_push(msg);
        return false;
    }

    /* Buffer for line in file */
    char line[600];
    uint32_t linear_base = 0;
    flash_page_t* page_cache = NULL;
    bool eof_found = false;

    /* Get every line from file */
    while (fgets(line, sizeof(line), file) != NULL)
    {
        /* Ignore lines thich not start with ':' */
        if (line[0] != ':') continue;

        /* Delete trailing whitespaces */
        size_t line_len = strlen(line);
        while ((line_len > 0) && ((line[line_len - 1] == '\n') || (line[line_len - 1] == '\r')))
        {
            line[--line_len] = '\0';
        }

        /* Check if line is valid */
        if (line_len < HEX_MINIMUM_LINE_LENGTH)
        {
            fclose(file); 
            free_page_list(out_pages);
            bridge_log_queue_push("Invalid HEX file, line too short");
            return false;
        }

        /* Parse line components */
        uint8_t byte_count;
        uint8_t rec_type;
        uint8_t addr_hi;
        uint8_t addr_lo;
        uint8_t checksum;
        if (!hex_byte_to_value(&line[1], &byte_count) || !hex_byte_to_value(&line[3], &addr_hi) ||
            !hex_byte_to_value(&line[5], &addr_lo) || !hex_byte_to_value(&line[7], &rec_type))
        {
            fclose(file); 
            free_page_list(out_pages);
            bridge_log_queue_push("Invalid HEX file, line header wrong");
            return false;
        }

        /* Line must be min. length + data bytes factor 2 */
        if (line_len != (HEX_MINIMUM_LINE_LENGTH + (size_t)byte_count * 2))
        {
            fclose(file); 
            free_page_list(out_pages);
            bridge_log_queue_push("Invalid HEX file, wrong line length");
            return false;
        }

        /* Parse data bytes */
        uint8_t data[HEX_MAXIMUM_LINE_DATA_LENGTH];
        uint32_t sum = (uint32_t)byte_count + addr_hi + addr_lo + rec_type;
        for (uint8_t byte_index = 0; byte_index < byte_count; byte_index++)
        {
            if (!hex_byte_to_value(&line[9 + (size_t)byte_index * 2], &data[byte_index]))
            {
                fclose(file); 
                free_page_list(out_pages);
                bridge_log_queue_push("Invalid HEX file, invalid data");
                return false;
            }
            sum += data[byte_index];
        }

        /* Parse checksum */
        if (!hex_byte_to_value(&line[9 + (size_t)byte_count * 2], &checksum))
        {
            fclose(file); 
            free_page_list(out_pages);
            bridge_log_queue_push("Invalid HEX file, checksum failed to parse");
            return false;
        }

        /* Check checksum */
        if ((uint8_t)(0x100 - (sum & 0xFF)) != checksum)
        {
            fclose(file); 
            free_page_list(out_pages);
            bridge_log_queue_push("Invalid HEX file, checksum mismatch");
            return false;
        }

        /* Get memory address for the line */
        uint16_t record_address = ((uint16_t)addr_hi << 8) | addr_lo;

        /* Add data to pages */
        if (rec_type == 0x00)
        {
            uint32_t abs_address = linear_base + record_address;
            for (uint8_t byte_index = 0; byte_index < byte_count; byte_index++)
            {
                uint32_t byte_address = abs_address + byte_index;
                uint32_t page_address = (byte_address / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
                uint32_t offset_in_page = byte_address % FLASH_PAGE_SIZE;

                /* Get or create page */
                flash_page_t* page = get_or_create_page(out_pages, &page_cache, page_address);

                /* Page failed */
                if (page == NULL)
                {
                    fclose(file); 
                    free_page_list(out_pages);
                    bridge_log_queue_push("HEX file, Internal error");
                    return false;
                }

                /* Add data to page */
                page->data[offset_in_page] = data[byte_index];
            }
        }
        else if (rec_type == 0x01)
        {
            eof_found = true;
            break;
        }
        else if (rec_type == 0x04)
        {
            if (byte_count != 2)
            {
                fclose(file); 
                free_page_list(out_pages);
                bridge_log_queue_push("HEX file, Internal error");
                return false;
            }
            linear_base = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16);
        }
        else if (rec_type == 0x02)
        {
            if (byte_count != 2)
            {
                fclose(file); 
                free_page_list(out_pages);
                bridge_log_queue_push("HEX file, Internal error");
                return false;
            }
            linear_base = (((uint32_t)data[0] << 8) | data[1]) * 16;
        }
    }

    fclose(file);

    if (!eof_found)
    {
        free_page_list(out_pages);
        bridge_log_queue_push("HEX file, Internal error");
        return false;
    }
    if (out_pages->page_count == 0)
    {
        bridge_log_queue_push("HEX file, Internal error");
        return false;
    }

	page_read_write_num = 0;
    pages_read_write_total = out_pages->page_count * 2;
    snprintf(msg, sizeof(msg), "Firmware file %s parsed successfully from path %s", filename, full_path);
    bridge_log_queue_push(msg);

    return true;
}

static bool acquire_port_for_flashing(uint16_t* out_port_number, int32_t* out_baud_rate)
{
    /* Check arguments */
    if (out_port_number == NULL) return false;
    if (out_baud_rate == NULL) return false;

    /* Get connection info */
    connection_info_t info;
    bridge_get_connection_info(&info);

    /* If no board is connected abort */
    if (!info.connected)
    {
        bridge_log_queue_push("No board connected");
        return false;
    }

    /* Copy connection info */
    *out_port_number = (uint16_t)info.port_number;
    *out_baud_rate = info.baud_rate;

    /* Wait for the sensor to disconnect */
    bridge_log_queue_push("Disconnect sensor connection...");
    bridge_publish_connection_request((uint16_t)info.port_number, info.baud_rate, false, false);

    /* Wait until sensor is disconnected */
    uint64_t wait_start = GetTickCount64();
    while (true)
    {
        bridge_get_connection_info(&info);
        if (!info.connected) break;
        if ((GetTickCount64() - wait_start) > FLASH_SENSOR_DISCONNECT_TIMEOUT_MS)
        {
            bridge_log_queue_push("Sensor connection could not be disconnected in time");
            return false;
        }
        Sleep(100);
    }

    /* Acquire serial port */
    if (!serial_port_acquire(SERIAL_SELF_ROLE, FLASH_ACQUIRE_TIMEOUT_MS))
    {
        bridge_log_queue_push("Serial port could not be acquired");
        return false;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Serial port (COM%d, Baud %d) successfully acquired", *out_port_number, *out_baud_rate);
    bridge_log_queue_push(msg);
    return true;
}

static bool perform_flash(const char* filename)
{
    if (filename == NULL) return false;

    /* Parse hex file */
    flash_set_progress(BRIDGE_FLASH_READ_FILE, 0);
    page_read_write_num = 0;
    pages_read_write_total = 0;
    flash_page_list_t pages;
    if (!parse_hex_file(filename, &pages)) return false;

    /*Acquire serial port for flashing */
    uint16_t port_number = 0;
    int32_t baud_rate = 0;
    if (!acquire_port_for_flashing(&port_number, &baud_rate))
    {
        free_page_list(&pages);
        return false;
    }

    bool success = false;
    uint8_t seq = 0;

    /* Check bootloader signature */
    flash_set_progress(BRIDGE_FLASH_CHECKING_SIGNATURE, 0);
    if (enter_bootloader_and_check_signature(port_number, baud_rate, &seq))
    {
        /* Write firmware */
        flash_set_progress(BRIDGE_FLASH_WRITING_FIRMWARE, 0);
        if (write_pages(&pages, &seq))
        {
            /* Verify firmware */
            flash_set_progress(BRIDGE_FLASH_VERIFYING_FIRMWARE, 0);
            success = verify_pages(&pages, &seq);

            if (success)
            {
                if (stk500_leave_progmode(&seq))
                {
                    bridge_log_queue_push("Leave programming mode, start application...");
                }
                else
                {
                    bridge_log_queue_push("Failed to leave programming mode");
                }
            }
        }
    }

    /* Release resources */
    bridge_publish_connection_info(false, -1, -1, false);
    serial_port_close(SERIAL_SELF_ROLE);
    serial_port_release(SERIAL_SELF_ROLE);
    free_page_list(&pages);

    return success;
}

static DWORD WINAPI worker_flash_thread_proc(LPVOID param)
{
    (void)param;

    while (InterlockedCompareExchange(&should_stop, 0, 0) == 0)
    {
        /* Check for flash requests */
        bridge_flash_params_t request;
        if (bridge_get_flash_request_if_changed(&request))
        {
            /* Log filename */
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Flash process started: %s", request.firmware_file);
            bridge_log_queue_push(log_msg);

            /* Flash firmware */
            bool success = perform_flash(request.firmware_file);

            /* Set flash progress */
            flash_set_progress(success ? BRIDGE_FLASH_COMPLETED : BRIDGE_FLASH_ERROR, success ? 100 : 0);
            bridge_log_queue_push(success ? "Flashed successfully" : "Flashed terminated with error");
        }
        Sleep(100);
    }
    return 0;
}

void worker_flash_start(void)
{
    should_stop = 0;
    thread_handle = CreateThread(NULL, 0, worker_flash_thread_proc, NULL, 0, NULL);
    return;
}

void worker_flash_stop(void)
{
    InterlockedExchange(&should_stop, 1);
    if (thread_handle != NULL)
    {
        WaitForSingleObject(thread_handle, 2000);
        CloseHandle(thread_handle);
        thread_handle = NULL;
    }
    return;
}