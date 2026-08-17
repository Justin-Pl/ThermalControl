/* Header */
#include "worker_sensor.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Defines */
/** @brief Maximum payload size carried by one frame, in bytes. */
#define MAX_PAYLOAD_SIZE            255

/** @brief Number of sync bytes at the start of every frame. */
#define SYNC_BYTE_SIZE              2

#define SYNC_PATTERN                0x55AA

/** @brief Size of the length field in the frame. */
#define LENGTH_SIZE                 2

#define PREAMBLE_SIZE               (SYNC_BYTE_SIZE + LENGTH_SIZE)  /**< Total size of the preamble in bytes. */

#define ID_SIZE                     8
#define TIMESTAMP_SIZE              8
#define STATUS_SIZE                 1
#define FLAGS_SIZE                  1
#define HEADER_SIZE                 (ID_SIZE + TIMESTAMP_SIZE + STATUS_SIZE + FLAGS_SIZE)  

/** @brief Size of the payload size field. */
#define PAYLOAD_SIZE_FIELD_SIZE         1

#define CHECKSUM_SIZE                 4

/** @brief Initial CRC32 register value (standard, polynomial 0xEDB88320). */
#define BASE_CRC32                  0xFFFFFFFF

/** @brief Return value of is_flag_valid() when the flag set is invalid. */
#define FLAG_INVALID                false

/** @brief Return value of is_flag_valid() when the flag set is valid. */
#define FLAG_VALID                  true

/* Register Address Space Layout */
/** @brief First valid address in the register space */
#define REGISTER_START               0x00

/** @brief First address of the system block */
#define REGISTER_SYSTEM_START        0x00

/** @brief Last address of the system block */
#define REGISTER_SYSTEM_END          0x1F

/** @brief First address of the time block */
#define REGISTER_TIME_START          0x20

/** @brief Last address of the time block */
#define REGISTER_TIME_END            0x3F

/** @brief First address of the input block */
#define REGISTER_IN_START            0x40

/** @brief Last address of the input block */
#define REGISTER_IN_END              0x5F

/** @brief First address of the output block */
#define REGISTER_OUT_START           0x60

/** @brief Last address of the output block */
#define REGISTER_OUT_END             0x6F

/** @brief First address of the configuration block */
#define REGISTER_CONFIG_START        0x70

/** @brief Last address of the configuration block */
#define REGISTER_CONFIG_END          0x7F

/** @brief First address of the calibration block */
#define REGISTER_CAL_START           0x80

/** @brief Last address of the calibration block */
#define REGISTER_CAL_END             0x9F

/** @brief First address of the debug block */
#define REGISTER_DEBUG_START         0xA0

/** @brief Last address of the debug block */
#define REGISTER_DEBUG_END           0xBF

/** @brief First address of the test block */
#define REGISTER_TEST_START          0xF0

/** @brief Last address of the test block */
#define REGISTER_TEST_END            0xFF

/** @brief Last valid address in the register space */
#define REGISTER_END                 0xFF

/** @brief Total size of the register address space in bytes */
#define REGISTER_SIZE                256

#define TIME_SYNC_RETRY_INTERVAL_MS 1000

#define AUTO_CONNECT_PING_RETRIES     3
#define AUTO_CONNECT_PING_TIMEOUT_MS  500
#define AUTO_CONNECT_RESET_WAIT_MS    3000

/* Type definitions */
typedef uint64_t  frame_id_t;        /**< Unique frame request identifier */
typedef uint64_t  utc_timestamp_t;   /**< UTC timestamp in milliseconds   */
typedef uint32_t  crc32_t;           /**< CRC32 checksum                  */
typedef uint8_t   flag_t;            /**< Frame flags bitfield            */

/**
 * @brief Receive state machine states.
 *
 * The receiver waits for the two sync bytes (0xAA, 0x55) and then
 * accumulates the rest of the frame until length and CRC have been
 * validated.
 */
typedef enum : uint8_t
{
    RX_WAIT_SYNC1,    /**< Waiting for first sync byte (0xAA)        */
    RX_WAIT_SYNC2,    /**< Waiting for second sync byte (0x55)       */
    RX_WAIT_FRAME     /**< Accumulating length, header, payload, CRC */
} rx_state_t;

/**
 * @brief Operation status codes used in the frame status field.
 */
typedef enum : uint8_t
{
    STATUS_OK = 0x00,   /**< Operation succeeded               */
    STATUS_ERR_CRC = 0x01,   /**< CRC mismatch (rarely sent)        */
    STATUS_ERR_INVALID_REG = 0x02,   /**< Invalid register or access type   */
    STATUS_ERR_INVALID_ARG = 0x03,   /**< Invalid payload format / length   */
    STATUS_ERR_GENERIC = 0xFF    /**< Generic / unspecified error       */
} status_t;

/**
 * @brief Bit masks for the flags field of a frame.
 *
 * Exactly one operation flag (READ, WRITE, TIME_SYNC, PING) must be
 * set per frame. The RESPONSE bit is added to mark a frame as a reply.
 */
typedef enum : uint8_t
{
    FLAG_MASK_READ = 0x01,   /**< Read register operation          */
    FLAG_MASK_WRITE = 0x02,   /**< Write register operation         */
    FLAG_MASK_RESPONSE = 0x04,   /**< Frame is a response, not request */
    FLAG_MASK_TIME_SYNC = 0x08,   /**< UTC time synchronization         */
    FLAG_MASK_PING = 0x10    /**< Connectivity check               */
} flag_mask_t;

/**
 * @brief Complete frame structure for in-memory representation.
 *
 * Used both for outgoing (send_frame) and incoming (process_frames)
 * frames. Fields are not packed - the binary on-the-wire format is
 * produced by send_frame() and parsed by the receive state machine.
 */
typedef struct
{
    /* Preamble */
    uint16_t        sync_bytes;       /**< Fixed pattern 0xAA, 0x55       */
    uint16_t        frame_length;     /**< Header + payload size + CRC    */

    /* Header */
    frame_id_t      id;               /**< Unique request id              */
    utc_timestamp_t timestamp;        /**< UTC time in ms (filled on send)*/
    status_t        status;           /**< Operation result               */
    flag_t          flags;            /**< Operation type bitmap          */

    /* Payload */
    uint8_t         payload_size;               /**< Length of the payload field */
    uint8_t         payload[MAX_PAYLOAD_SIZE];  /**< Operation-specific data */

    /* Checksum */
    crc32_t         crc32;            /**< CRC32 over Sync..Payload       */
} frame_t;

typedef struct
{
    rx_state_t state;
    uint8_t raw_buffer[PREAMBLE_SIZE + HEADER_SIZE + PAYLOAD_SIZE_FIELD_SIZE + MAX_PAYLOAD_SIZE + CHECKSUM_SIZE];   /* Praeambel+Header+Payload+CRC max */
    size_t bytes_needed;    /* wie viele Bytes insgesamt fuer den aktuellen Frame erwartet werden */
    size_t bytes_collected;
} rx_context_t;

typedef enum
{
    /* System (0x00 - 0x1F) */
    REG_SYS_FW_VERSION_MAJOR = REGISTER_SYSTEM_START,
    REG_SYS_FW_VERSION_MINOR,
    REG_SYS_FW_VERSION_PATCH,
    REG_SYS_RST_DEVICE,
    REG_SYS_LAST_ERR_CODE,
    REG_SYS_CLEAR_ERRS,
    REG_SYS_ERR_COUNT_1,
    REG_SYS_ERR_COUNT_2,
    REG_SYS_ERR_COUNT_3,
    REG_SYS_ERR_COUNT_4,

    /* Time (0x20 - 0x3F) */
    REG_TIME_UPTIME_1 = REGISTER_TIME_START,
    REG_TIME_UPTIME_2,
    REG_TIME_UPTIME_3,
    REG_TIME_UPTIME_4,
    REG_TIME_UPTIME_5,
    REG_TIME_UPTIME_6,
    REG_TIME_UPTIME_7,
    REG_TIME_UPTIME_8,
    REG_TIME_UTC_1,
    REG_TIME_UTC_2,
    REG_TIME_UTC_3,
    REG_TIME_UTC_4,
    REG_TIME_UTC_5,
    REG_TIME_UTC_6,
    REG_TIME_UTC_7,
    REG_TIME_UTC_8,
    REG_TIME_OFFSET_1,
    REG_TIME_OFFSET_2,
    REG_TIME_OFFSET_3,
    REG_TIME_OFFSET_4,
    REG_TIME_OFFSET_5,
    REG_TIME_OFFSET_6,
    REG_TIME_OFFSET_7,
    REG_TIME_OFFSET_8,
    REG_TIME_SYNC_AGE_1,
    REG_TIME_SYNC_AGE_2,
    REG_TIME_SYNC_AGE_3,
    REG_TIME_SYNC_AGE_4,

    /* Input - (0x40 - 0x5F) */
    REG_IN_TEMP_RAW_1 = REGISTER_IN_START,
    REG_IN_TEMP_RAW_2,
    REG_IN_TEMP_CAL_1,
    REG_IN_TEMP_CAL_2,
    REG_IN_READING_AGE_1,
    REG_IN_READING_AGE_2,
    REG_IN_READING_AGE_3,
    REG_IN_READING_AGE_4,
    REG_IN_DATA_FRESH,
    REG_IN_SENSOR_STATUS,
    REG_IN_READ_COUNT_1,
    REG_IN_READ_COUNT_2,
    REG_IN_READ_COUNT_3,
    REG_IN_READ_COUNT_4,
    REG_IN_FAIL_COUNT_1,
    REG_IN_FAIL_COUNT_2,
    REG_IN_FAIL_COUNT_3,
    REG_IN_FAIL_COUNT_4,

    /* Output - (0x60 - 0x6F) */
    REG_OUT_MOSFET_PWM = REGISTER_OUT_START,
    REG_OUT_MOSFET_ENABLE,

    /* Config (0x70 - 0x7F) */
    REG_CONFIG_SAVE = REGISTER_CONFIG_START,
    REG_CONFIG_LOAD_DEFAULTS,

    /* Calibration (0x80 - 0x9F) */
    REG_CAL_TEMP_OFFSET_1 = REGISTER_CAL_START,
    REG_CAL_TEMP_OFFSET_2,
    REG_CAL_TEMP_GAIN_1,
    REG_CAL_TEMP_GAIN_2,
    REG_CAL_UTC_DATE_1,
    REG_CAL_UTC_DATE_2,
    REG_CAL_UTC_DATE_3,
    REG_CAL_UTC_DATE_4,
    REG_CAL_UTC_DATE_5,
    REG_CAL_UTC_DATE_6,
    REG_CAL_UTC_DATE_7,
    REG_CAL_UTC_DATE_8,

    /* Debug (0xA0 - 0xBF) */
    REG_DEBUG_CRC_ERR_COUNT_1 = REGISTER_DEBUG_START,
    REG_DEBUG_CRC_ERR_COUNT_2,
    REG_DEBUG_TIMEOUT_COUNT_1,
    REG_DEBUG_TIMEOUT_COUNT_2,
    REG_DEBUG_UART_OVERRUN_1,
    REG_DEBUG_UART_OVERRUN_2,
    REG_DEBUG_GEN_RX_ERR_COUNT_1,
    REG_DEBUG_GEN_RX_ERR_COUNT_2,
    REG_DEBUG_RX_PACKET_COUNT_1,
    REG_DEBUG_RX_PACKET_COUNT_2,
    REG_DEBUG_RX_PACKET_COUNT_3,
    REG_DEBUG_RX_PACKET_COUNT_4,
    REG_DEBUG_TX_PACKET_COUNT_1,
    REG_DEBUG_TX_PACKET_COUNT_2,
    REG_DEBUG_TX_PACKET_COUNT_3,
    REG_DEBUG_TX_PACKET_COUNT_4,
    REG_DEBUG_LAST_REQ_AGE_1,
    REG_DEBUG_LAST_REQ_AGE_2,
    REG_DEBUG_LAST_REQ_AGE_3,
    REG_DEBUG_LAST_REQ_AGE_4,

    /* Test (0xF0 - 0xFF) */
    REG_TEST_FORCE_ERROR = REGISTER_TEST_START,
} register_t;

typedef enum 
{ 
    SENSOR_STATE_IDLE, 
    SENSOR_STATE_CONNECTED 
} sensor_state_t;

/* Static local variables */
static uint32_t crc32_table[256];
static bool crc32_table_ready = false;
static frame_id_t next_request_id = 1;
static volatile LONG should_stop = 0;
static HANDLE thread_handle = NULL;
static sensor_state_t sensor_state = SENSOR_STATE_IDLE;
static uint16_t current_port = 0;
static int32_t current_baud = 0;
static bool time_synced = false;
static uint64_t last_time_sync_attempt_ms = 0;
static float pid_integral_accumulator = 0.0f;
static float pid_last_error = 0.0f;
static uint64_t pid_last_compute_ms = 0;
static bridge_control_config_t control_config;

/* Debug */
#define SERIAL_DEBUG_ENABLED 0

#if SERIAL_DEBUG_ENABLED

static void debug_dump_hex(const char* label, const uint8_t* data, size_t length)
{
    printf("[%s] %zu Bytes:\n", label, length);
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");
    return;
}

static void debug_dump_frame(const char* label, const frame_t* frame)
{
    printf("[%s] Frame:\n", label);
    printf("  sync_bytes   = 0x%04X\n", frame->sync_bytes);
    printf("  frame_length = %u\n", frame->frame_length);
    printf("  id           = %llu\n", (unsigned long long)frame->id);
    printf("  timestamp    = %llu\n", (unsigned long long)frame->timestamp);
    printf("  status       = 0x%02X\n", frame->status);
    printf("  flags        = 0x%02X\n", frame->flags);
    printf("  payload_size = %u\n", frame->payload_size);
    printf("  payload      = ");
    for (uint8_t i = 0; i < frame->payload_size; i++)
    {
        printf("%02X ", frame->payload[i]);
    }
    printf("\n  crc32        = 0x%08X\n\n", frame->crc32);
    return;
}

#else

static void debug_dump_hex(const char* label, const uint8_t* data, size_t length) { (void)label; (void)data; (void)length; }
static void debug_dump_frame(const char* label, const frame_t* frame) { (void)label; (void)frame; }

#endif

/* Static function definitions */
static uint64_t get_current_utc_ms(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli = { 0 };
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    uint64_t ms_since_1601 = uli.QuadPart / 10000ULL;
    const uint64_t EPOCH_DIFF_MS = 11644473600000ULL;
    return ms_since_1601 - EPOCH_DIFF_MS;
}

static void crc32_init_table(void)
{
    for (size_t byte_index = 0; byte_index < 256; byte_index++)
    {
        uint32_t crc = byte_index;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
        }
        crc32_table[byte_index] = crc;
    }
    crc32_table_ready = true;

    return;
}

static crc32_t crc32_compute(const uint8_t* data, size_t length)
{
	if ((data == NULL) || !length) return 0;
    if (!crc32_table_ready) crc32_init_table();

    uint32_t crc = BASE_CRC32;
    for (size_t byte_index = 0; byte_index < length; byte_index++)
    {
        crc = crc32_table[(crc ^ data[byte_index]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ BASE_CRC32;
}

static void write_u16_le(uint8_t* dst, uint16_t value)
{
    if (dst == NULL) return;

    dst[0] = (uint8_t)(value & 0xFF);
    dst[1] = (uint8_t)((value >> 8) & 0xFF);
    return;
}

static void write_u32_le(uint8_t* dst, uint32_t value)
{
    if (dst == NULL) return;

    dst[0] = (uint8_t)(value & 0xFF);
    dst[1] = (uint8_t)((value >> 8) & 0xFF);
    dst[2] = (uint8_t)((value >> 16) & 0xFF);
    dst[3] = (uint8_t)((value >> 24) & 0xFF);
    return;
}

static void write_u64_le(uint8_t* dst, uint64_t value)
{
    if (dst == NULL) return;

    for (int byte_index = 0; byte_index < 8; byte_index++)
    {
        dst[byte_index] = (uint8_t)((value >> (byte_index * 8)) & 0xFF);
    }
    return;
}

static uint16_t read_u16_le(const uint8_t* src)
{
    if (src == NULL) return 0;

    return (uint16_t)(src[0] | (src[1] << 8));
}

static uint32_t read_u32_le(const uint8_t* src)
{
    if (src == NULL) return 0;

    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static uint64_t read_u64_le(const uint8_t* src)
{
    if (src == NULL) return 0;

    uint64_t value = 0;
    for (int byte_index = 0; byte_index < 8; byte_index++)
    {
        value |= ((uint64_t)src[byte_index]) << (byte_index * 8);
    }
    return value;
}

static int16_t read_i16_le(const uint8_t* src)
{
    if (src == NULL) return 0;

    return (int16_t)read_u16_le(src);
}

static bool send_frame(serial_port_owner_t self, frame_id_t id, utc_timestamp_t timestamp,
    flag_mask_t flags, const uint8_t* payload, uint8_t payload_size)
{
    if (payload_size > MAX_PAYLOAD_SIZE) return false;

    /* Header (28 Byte) + Payload-Size-Feld (1) + Payload + CRC (4) */
    uint8_t header_and_payload[HEADER_SIZE + PAYLOAD_SIZE_FIELD_SIZE + MAX_PAYLOAD_SIZE] = { 0 };
    size_t offset = 0;

    write_u64_le(&header_and_payload[offset], id); offset += ID_SIZE;
    write_u64_le(&header_and_payload[offset], timestamp); offset += TIMESTAMP_SIZE;
    header_and_payload[offset++] = STATUS_OK;   /* Status im Request irrelevant, aber muss belegt sein */
    header_and_payload[offset++] = (uint8_t)flags;
    header_and_payload[offset++] = payload_size;

    if ((payload_size > 0) && (payload != NULL))
    {
        memcpy(&header_and_payload[offset], payload, payload_size);
    }
    offset += payload_size;

    /* Laut Datenblatt: 'Length' zaehlt Header+Payload+Checksum (NICHT die Praeambel selbst) */
    uint16_t length_field = (uint16_t)(offset + sizeof(crc32_t));

    /* Kompletten Frame auf dem Draht zusammenbauen: Sync+Length (Praeambel) + alles oben + CRC */
    uint8_t wire_buffer[PREAMBLE_SIZE + sizeof(header_and_payload) + CHECKSUM_SIZE] = { 0 };
    size_t wire_offset = 0;

    write_u16_le(&wire_buffer[wire_offset], SYNC_PATTERN); wire_offset += SYNC_BYTE_SIZE;
    write_u16_le(&wire_buffer[wire_offset], length_field); wire_offset += LENGTH_SIZE;

    memcpy(&wire_buffer[wire_offset], header_and_payload, offset);
    wire_offset += offset;

    crc32_t crc = crc32_compute(wire_buffer, wire_offset);

    write_u32_le(&wire_buffer[wire_offset], crc);
    wire_offset += sizeof(crc32_t);

    debug_dump_hex("TX RAW", wire_buffer, wire_offset);

    size_t written = serial_port_write(self, wire_buffer, wire_offset);
    return written == wire_offset;
}

static void rx_context_reset(rx_context_t* context)
{
    if (context == NULL) return;

    context->state = RX_WAIT_SYNC1;
    context->bytes_collected = 0;
    context->bytes_needed = 0;
    return;
}

static bool receive_frame(serial_port_owner_t self, frame_t* out_frame, uint32_t timeout_ms)
{
    if (out_frame == NULL) return false;    

    rx_context_t context;
    rx_context_reset(&context);
    uint16_t length_field = 0;

    uint64_t start_ms = get_current_utc_ms();

    while (true)
    {
        uint64_t elapsed_ms = get_current_utc_ms() - start_ms;
        if (elapsed_ms >= timeout_ms) return false;   /* Gesamt-Budget aufgebraucht */

        uint32_t remaining_ms = (uint32_t)(timeout_ms - elapsed_ms);

        uint8_t byte;
        size_t read_count = serial_port_read(self, &byte, 1, remaining_ms);   /* schrumpfendes Restbudget */
        if (read_count == 0) return false;

        switch (context.state)
        {
        case RX_WAIT_SYNC1:
            if (byte == (SYNC_PATTERN & 0xFF))
            {
                context.raw_buffer[0] = byte;
                context.bytes_collected = 1;
                context.state = RX_WAIT_SYNC2;
            }
            break;

        case RX_WAIT_SYNC2:
            if (byte == ((SYNC_PATTERN >> 8) & 0xFF))
            {
                context.raw_buffer[1] = byte;
                context.bytes_collected = 2;
                context.state = RX_WAIT_FRAME;
            }
            else
            {
                rx_context_reset(&context);
            }
            break;

        case RX_WAIT_FRAME:
            context.raw_buffer[context.bytes_collected++] = byte;

            if (context.bytes_collected == PREAMBLE_SIZE)
            {
                length_field = read_u16_le(&context.raw_buffer[2]);
                context.bytes_needed = PREAMBLE_SIZE + length_field;

                if (context.bytes_needed > sizeof(context.raw_buffer))
                {
                    rx_context_reset(&context);
                    continue;
                }
            }

            if ((context.bytes_needed > 0) && (context.bytes_collected == context.bytes_needed))
            {
                debug_dump_hex("RX RAW", context.raw_buffer, context.bytes_needed);

                size_t crc_offset = context.bytes_needed - CHECKSUM_SIZE;
                crc32_t received_crc = read_u32_le(&context.raw_buffer[crc_offset]);
                crc32_t computed_crc = crc32_compute(context.raw_buffer, crc_offset);

                if (received_crc != computed_crc)
                {
                    rx_context_reset(&context);
                    continue;
                }

                /* Entpacken - Reihenfolge exakt wie in Tabelle 4.1 */
                size_t byte_index = PREAMBLE_SIZE;
                out_frame->sync_bytes = SYNC_PATTERN;
                out_frame->frame_length = length_field;

                out_frame->id = read_u64_le(&context.raw_buffer[byte_index]); byte_index += ID_SIZE;
                out_frame->timestamp = read_u64_le(&context.raw_buffer[byte_index]); byte_index += TIMESTAMP_SIZE;
                out_frame->status = (status_t)context.raw_buffer[byte_index]; byte_index += STATUS_SIZE;
                out_frame->flags = context.raw_buffer[byte_index]; byte_index += FLAGS_SIZE;
                out_frame->payload_size = context.raw_buffer[byte_index]; byte_index += PAYLOAD_SIZE_FIELD_SIZE;

                if (out_frame->payload_size > MAX_PAYLOAD_SIZE)
                {
                    rx_context_reset(&context);
                    continue;
                }

                memcpy(out_frame->payload, &context.raw_buffer[byte_index], out_frame->payload_size);
                byte_index += out_frame->payload_size;

                out_frame->crc32 = received_crc;

                return true;
            }
            break;
        }
    }
}

static bool read_registers(serial_port_owner_t self, register_t start_reg, uint8_t count, uint8_t* out_data)
{
    if (out_data == NULL) return false;

    uint8_t payload[2] = { (uint8_t)start_reg, count };
    frame_id_t id = next_request_id++;
    uint64_t now_ms = get_current_utc_ms();

    if (!send_frame(self, id, (utc_timestamp_t)now_ms, FLAG_MASK_READ, payload, sizeof(payload))) return false;

    frame_t response;
    if (!receive_frame(self, &response, 500)) return false;

    if (response.id != id) return false;             /* falsche Antwort zugeordnet */
    if (response.status != STATUS_OK) return false;   /* Sensor meldet Fehler */
    if (response.payload_size != (count + 2)) return false;

    size_t data_size = response.payload_size;
    data_size -= 2;
    memcpy(out_data, &response.payload[2], data_size);
    return true;
}

static bool write_registers(serial_port_owner_t self, register_t start_reg, const uint8_t* data, uint8_t length)
{
    if ((data == NULL) || !length) return false;

    if (length > (MAX_PAYLOAD_SIZE - 2)) return false;   /* 2 Byte fuer start_addr+length reserviert */

    uint8_t payload[2 + MAX_PAYLOAD_SIZE] = { 0 };
    payload[0] = (uint8_t)start_reg;
    payload[1] = length;
    memcpy(&payload[2], data, length);

    frame_id_t id = next_request_id++;
    uint64_t now_ms = get_current_utc_ms();
    if (!send_frame(self, id, 0, FLAG_MASK_WRITE, payload, (uint8_t)(2 + length))) return false;

    frame_t response;
    if (!receive_frame(self, &response, 500)) return false;

    return (response.id == id) && (response.status == STATUS_OK);
}

static bool send_time_sync(serial_port_owner_t self)
{
    uint64_t now_ms = get_current_utc_ms();   /* GetSystemTimeAsFileTime, bereits vorhanden */
    frame_id_t id = next_request_id++;

    uint8_t payload[8];
    write_u64_le(payload, now_ms);

    if (!send_frame(self, id, (utc_timestamp_t)now_ms, FLAG_MASK_TIME_SYNC, payload, sizeof(payload))) return false;

    frame_t response;
    if (!receive_frame(self, &response, 500)) return false;

    return (response.id == id) && (response.status == STATUS_OK);
}

static bool poll_sensor_data(serial_port_owner_t self, float* out_temp_c, uint8_t* out_pwm)
{
    if (out_temp_c == NULL) return false;
    if (out_pwm == NULL) return false;

    uint8_t status_byte;
    if (!read_registers(self, REG_IN_DATA_FRESH, 1, &status_byte)) return false;

    if (status_byte == 0) return false;   /* noch kein neuer Messwert */

    /* Lesezugriff auf das Sensorregister setzt laut Datenblatt automatisch
       das Statusbit zurueck - kein separates WRITE noetig */
    uint8_t temp_bytes[2];
    if (!read_registers(self, REG_IN_TEMP_CAL_1, 2, temp_bytes)) return false;

    int16_t raw_temp = read_i16_le(temp_bytes);
    *out_temp_c = (float)raw_temp / 100.0f;   /* Faktor 100 laut Datenblatt (1 C = 100) */

    uint8_t pwm_byte;
    if (!read_registers(self, REG_OUT_MOSFET_PWM, 1, &pwm_byte)) return false;
    *out_pwm = pwm_byte;

    return true;
}

static bool send_ping(serial_port_owner_t self)
{
    frame_id_t id = next_request_id++;
    uint64_t now_ms = get_current_utc_ms();

    if (!send_frame(self, id, (utc_timestamp_t)now_ms, FLAG_MASK_PING, NULL, 0)) return false;

    frame_t response;
    if (!receive_frame(self, &response, AUTO_CONNECT_PING_TIMEOUT_MS)) return false;

    return (response.id == id) && (response.status == STATUS_OK);
}

static bool try_port_for_auto_connect(uint16_t port_number, int32_t baud_rate)
{
    char port_name[16];
    snprintf(port_name, sizeof(port_name), "COM%u", port_number);

    char message[64];
    if (!serial_port_open(PORT_SENSOR, port_name, (uint32_t)baud_rate))
    {
        snprintf(message, sizeof(message), "Failed to open COM%u!", port_number);
        bridge_log_queue_push(message);
        return false;
    }

    Sleep(AUTO_CONNECT_RESET_WAIT_MS);
    serial_port_flush(PORT_SENSOR);

    bool found = false;
    for (int attempt = 0; (attempt < AUTO_CONNECT_PING_RETRIES) && !found; attempt++)
    {
        snprintf(message, sizeof(message), "Attempt (%u/%u) Send ping to COM%u...", attempt + 1, AUTO_CONNECT_PING_RETRIES, port_number);
        bridge_log_queue_push(message);
        found = send_ping(PORT_SENSOR);
        if (!found)
        {
            snprintf(message, sizeof(message), "No response from COM%u...", port_number);
            bridge_log_queue_push(message);
        }
    }

    if (!found)
    {
        serial_port_close(PORT_SENSOR);
    }
    else
    {
        snprintf(message, sizeof(message), "Device found on COM%u", port_number);
        bridge_log_queue_push(message);
    }

    return found;
}

static bool auto_connect_scan(int32_t baud_rate, uint16_t* out_found_port)
{
    if (out_found_port == NULL) return false;

    com_port_list_t port_list = { 0 };
    bridge_get_com_ports_if_changed(&port_list);

    if (port_list.com_count == 0)
    {
        bridge_log_queue_push("Auto-Connect: No COM-Ports available");
        return false;
    }

    for (size_t port_index = 0; port_index < port_list.com_count; port_index++)
    {
        uint16_t candidate = port_list.com_numbers[port_index];

        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "Auto-Connect: Testing COM%u...", candidate);
        bridge_log_queue_push(log_msg);

        if (try_port_for_auto_connect(candidate, baud_rate))
        {
            *out_found_port = candidate;
            return true;
        }
    }

    bridge_log_queue_push("Auto-Connect: No device found");
    return false;
}

static float pid_compute(const bridge_control_config_t* config, float current_temp_c)
{
    if (config == NULL) return 0.0f;

    /* Get delta time from last timestamp */
    uint64_t now_ms = get_current_utc_ms();
    float delta_time_s = (pid_last_compute_ms == 0) ? 0.1f : (float)(now_ms - pid_last_compute_ms) / 1000.0f;
    pid_last_compute_ms = now_ms;

    float error = config->pid_setpoint_c - current_temp_c;

    float p_term = config->pid_p_enabled ? (config->pid_kp * error) : 0.0f;

    if (config->pid_i_enabled)
    {
        pid_integral_accumulator += error * delta_time_s;
    }
    else
    {
        pid_integral_accumulator = 0.0f;
    }
    float i_term = config->pid_i_enabled ? (config->pid_ki * pid_integral_accumulator) : 0.0f;

    float derivative = (delta_time_s > 0.0f) ? ((error - pid_last_error) / delta_time_s) : 0.0f;
    float d_term = config->pid_d_enabled ? (config->pid_kd * derivative) : 0.0f;
    pid_last_error = error;

    float output = p_term + i_term + d_term;
    if (output < 0.0f) output = 0.0f;
    if (output > 100.0f) output = 100.0f;

    return output;
}

static float compute_target_pwm_percent(const bridge_control_config_t* config, float current_temp_c)
{
    /* Check argument */
    if (config == NULL) return 0.0f;

    switch (config->mode)
    {
    case BRIDGE_CONTROL_MODE_MANUAL:
        return config->manual_pwm_percent;

    case BRIDGE_CONTROL_MODE_TWO_POINT:
    {
        static bool two_point_output_on = false;
        float on_threshold = config->two_point_setpoint_c - (config->two_point_hysteresis_c / 2.0f);
        float off_threshold = config->two_point_setpoint_c + (config->two_point_hysteresis_c / 2.0f);

        if (current_temp_c < on_threshold) two_point_output_on = true;
        else if (current_temp_c > off_threshold) two_point_output_on = false;

        return two_point_output_on ? 100.0f : 0.0f;
    }

    case BRIDGE_CONTROL_MODE_PID:
    {
        return pid_compute(config, current_temp_c);
    }

    case BRIDGE_CONTROL_MODE_NONE:
        return 0.0f;
    default:
        return 0.0f;
    }
}

static DWORD WINAPI worker_sensor_thread_proc(LPVOID param)
{
    (void)param;

    while (InterlockedCompareExchange(&should_stop, 0, 0) == 0)
    {
        connection_request_t request;
        bool got_request = bridge_get_connection_request_if_changed(&request);

        if (sensor_state == SENSOR_STATE_IDLE)
        {
            if (got_request && request.connect_requested)
            {
                if (!serial_port_acquire(PORT_SENSOR, 1000))
                {
                    bridge_publish_error(ERROR_GENERAL, "Can't acquire serial port");
                    Sleep(200);
                    continue;
                }
                bridge_log_queue_push("Serial port acquired");

                uint16_t connected_port = 0;
                bool connection_established = false;

                if (request.auto_connect)
                {
                    bridge_publish_connection_info(false, 0, 0, true);
                    connection_established = auto_connect_scan(request.baud_rate, &connected_port);
                }
                else
                {
                    char port_name[16];
                    snprintf(port_name, sizeof(port_name), "COM%u", request.port_number);
                    connection_established = serial_port_open(PORT_SENSOR, port_name, (uint32_t)request.baud_rate);
                    connected_port = request.port_number;
                }


                if (!connection_established)
                {
                    if (!request.auto_connect) bridge_publish_error(ERROR_GENERAL, "Failed to open serial port!");
                    bridge_publish_connection_info(false, -1, -1, false);
                    serial_port_release(PORT_SENSOR);
                    Sleep(200);
                    continue;
                }
   
                char message[64];
                snprintf(message, sizeof(message), "Connected to COM%u, %lu baud", connected_port, request.baud_rate);
                bridge_log_queue_push(message);
               
                current_port = request.auto_connect ? connected_port : request.port_number;
                current_baud = request.baud_rate;
                bridge_publish_connection_info(true, current_port, current_baud, false);
                sensor_state = SENSOR_STATE_CONNECTED;
                time_synced = false;
                last_time_sync_attempt_ms = 0;
                
                if (!request.auto_connect)
                {
                    serial_port_flush(PORT_SENSOR);
                    Sleep(3000);
                }
            }
            Sleep(100);
        }
        else /* SENSOR_STATE_CONNECTED */
        {
            if (got_request && !request.connect_requested)
            {
                uint8_t disable_value = 0;
                write_registers(PORT_SENSOR, REG_OUT_MOSFET_ENABLE, &disable_value, 1);

                serial_port_close(PORT_SENSOR);
                serial_port_release(PORT_SENSOR);
                bridge_publish_connection_info(false, -1, -1, false);
                sensor_state = SENSOR_STATE_IDLE;
                time_synced = false;

                char message[64];
                snprintf(message, sizeof(message), "Connection to COM%u, %lu baud, terminated", current_port, current_baud);
                bridge_log_queue_push(message);

                continue;
            }

            if (!time_synced)
            {
                uint64_t now = get_current_utc_ms();
                if ((now - last_time_sync_attempt_ms) >= TIME_SYNC_RETRY_INTERVAL_MS)
                {
                    last_time_sync_attempt_ms = now;

                    serial_port_flush(PORT_SENSOR);
                    if (send_time_sync(PORT_SENSOR))
                    {
                        time_synced = true;
                        bridge_log_queue_push("Time sync successfull");

                        uint8_t enable_value = 1;
                        if (!write_registers(PORT_SENSOR, REG_OUT_MOSFET_ENABLE, &enable_value, 1))
                        {
                            bridge_log_queue_push("Failed to enable mosfet!");
                        }
                    }
                    else
                    {
                        bridge_log_queue_push("Time sync failed, next try in 1s...");
                    }
                }
                Sleep(100);
                continue;  
            }

            float temp_c;
            uint8_t pwm;
            if (bridge_get_control_config(&control_config))
            {
                if ((control_config.mode == BRIDGE_CONTROL_MODE_MANUAL) || (control_config.mode == BRIDGE_CONTROL_MODE_NONE))
                {
                    float target_percent = compute_target_pwm_percent(&control_config, temp_c);
                    uint8_t target_pwm_byte = (uint8_t)((target_percent / 100.0f) * 255.0f);
                    if (!write_registers(PORT_SENSOR, REG_OUT_MOSFET_PWM, &target_pwm_byte, 1))
                    {
                        bridge_log_queue_push("Error! Failed to set PWM value");
                    }
                }
            }

            if (poll_sensor_data(PORT_SENSOR, &temp_c, &pwm))
            {
                graph_point_t point = { 0 };
                point.timestamp_ms = get_current_utc_ms();
                point.temp = temp_c;
                point.pwm = (float)pwm / (float)UINT8_MAX * 100.0f;
                bridge_graph_queue_push(point);

                char point_log[64];
                snprintf(point_log, sizeof(point_log), "Temp.: %.2f C, PWM: %.0f%%", temp_c, point.pwm);
                bridge_log_queue_push(point_log);

                float target_percent = compute_target_pwm_percent(&control_config, temp_c);
                uint8_t target_pwm_byte = (uint8_t)((target_percent / 100.0f) * 255.0f);
                if (!write_registers(PORT_SENSOR, REG_OUT_MOSFET_PWM, &target_pwm_byte, 1))
                {
                    bridge_log_queue_push("Error! Failed to set PWM value");
                }
            }


            Sleep(100);
        }
    }

    if (sensor_state == SENSOR_STATE_CONNECTED)
    {
        uint8_t disable_value = 0;
        write_registers(PORT_SENSOR, REG_OUT_MOSFET_ENABLE, &disable_value, 1);

        serial_port_close(PORT_SENSOR);
        serial_port_release(PORT_SENSOR);
    }
    return 0;
}

void worker_sensor_start(void)
{
    should_stop = 0;
    thread_handle = CreateThread(NULL, 0, worker_sensor_thread_proc, NULL, 0, NULL);
    return;
}

void worker_sensor_stop(void)
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