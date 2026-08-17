/**
 * @file    communication.cpp
 * @brief   Serial communication implementation: framing, RX state machine,
 *          TX serializer, request dispatch.
 *
 * Implements the binary protocol layer between the host PC and the
 * firmware. Outgoing frames are serialized in send_frame(), incoming
 * frames are accumulated by poll_serial() and dispatched in
 * process_frames(). All multi-byte fields are little-endian.
 *
 * @author  Justin Plobst
 * @date    2026
 */

/* Header */
#include "communication.h"

/* Constant variables */
/**
 * @brief Sync byte sequence at the start of every frame.
 *
 * Stored as uint16_t in little-endian byte order, so the bytes 0xAA
 * followed by 0x55 are placed on the wire correctly when transmitted
 * via memcpy().
 */
const uint16_t SYNC_BYTES = 0x55AA;  /* Little-Endian Layout: 0xAA, 0x55 */

/**
 * @brief CRC32 lookup table (polynomial 0xEDB88320).
 *
 * Stored in PROGMEM to keep it out of SRAM. Access via pgm_read_dword().
 */
const crc32_t CRC32_TABLE[] PROGMEM = // CRC32 Lookup-Table (Polynomial: 0xEDB88320)
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/* Static variables */
/** @brief Buffer for outgoing frames (serialized for Serial.write). */
static uint8_t tx_buffer[MAX_FRAME_SIZE];

/** @brief Working buffer accumulating bytes of the currently received frame. */
static uint8_t rx_buffer[MAX_FRAME_SIZE];

/** @brief Current state of the receive state machine. */
static rx_state_t rx_state = RX_WAIT_SYNC1;

/** @brief Number of bytes accumulated in rx_buffer for the current frame. */
static uint16_t rx_working_offset = 0;

/** @brief Total expected frame size (preamble + frame_length) once known. */
static uint16_t rx_expected_length = 0;

/** @brief Uptime (ms) at which the current frame began arriving. */
static uint64_t rx_frame_start_uptime = 0;

/** @brief Ring buffer of validated frames awaiting processing. */
static frame_t rx_ring[RX_RING_CAPACITY];

/** @brief Ring buffer write index (where the next frame will be inserted). */
static uint8_t rx_ring_head = 0;

/** @brief Ring buffer read index (from which the next frame will be popped). */
static uint8_t rx_ring_tail = 0;

/** @brief Current number of frames stored in the ring buffer. */
static uint8_t rx_ring_count = 0;

/* Static helper definitions */
/**
 * @brief Reset the receive state machine to wait for the next sync byte.
 *
 * Called after a successful frame, on validation failure, or on timeout.
 */
static void rx_reset_state(void)
{
    rx_state = RX_WAIT_SYNC1;
    rx_working_offset = 0;
    rx_expected_length = 0;
    rx_frame_start_uptime = 0;
    memset(tx_buffer, 0x00, sizeof(tx_buffer));
    memset(rx_buffer, 0x00, sizeof(rx_buffer));
    return;
}

/**
 * @brief Append a validated frame to the receive ring buffer.
 * @param frame  Pointer to a fully populated frame
 * @return true on success, false if the ring is full or @p frame is NULL
 */
static bool rx_ring_push(const frame_t* frame)
{
    /* Check if frame is valid */
    if (frame == NULL) return false;

    /* Check if ring is full */
    if (rx_ring_count >= RX_RING_CAPACITY) return false;

    /* Copy frame into ring */
    memcpy(&rx_ring[rx_ring_head], frame, sizeof(frame_t));
    rx_ring_head = (rx_ring_head + 1) % RX_RING_CAPACITY;
    rx_ring_count++;

    return true;
}

/**
 * @brief Remove the oldest frame from the receive ring buffer.
 * @param frame  Destination buffer (must be non-NULL)
 * @return true on success, false if ring is empty or @p frame is NULL
 */
static bool rx_ring_pop(frame_t* frame)
{
    /* Check if frame is valid */
    if (frame == NULL) return false;

    /* Check if ring is empty */
    if (!rx_ring_count) return false;

    /* Copy frame from ring */
    memcpy(frame, &rx_ring[rx_ring_tail], sizeof(frame_t));
    rx_ring_tail = (rx_ring_tail + 1) % RX_RING_CAPACITY;
    rx_ring_count--;

    return true;
}

/**
 * @brief Validate the CRC of the current rx_buffer and store the frame.
 *
 * Called once a complete frame has been accumulated. On CRC mismatch,
 * the frame is discarded and the CRC error counter is incremented.
 * On success, the frame is parsed into a frame_t and pushed to the
 * ring buffer.
 */
static void rx_validate_and_store(void)
{
    /* Compute CRC over everything except the trailing CRC bytes */
    uint16_t crc_data_length = rx_working_offset - CHECKSUM_SIZE;
    crc32_t computed_crc = calc_crc32(rx_buffer, crc_data_length);

    /* Extract received CRC */
    crc32_t received_crc;
    memcpy(&received_crc, rx_buffer + crc_data_length, sizeof(received_crc));

    /* Validate CRC */
    if (computed_crc != received_crc)
    {
        /* Increase crc fail count */
        increment_register_u16(REG_DEBUG_CRC_ERR_COUNT_1);

        rx_reset_state();
        return;
    }

    /* Parse working buffer into frame_t */
    frame_t frame;
    uint16_t parse_offset = 0;

    memcpy(&frame.sync_bytes, rx_buffer + parse_offset, sizeof(frame.sync_bytes));
    parse_offset += sizeof(frame.sync_bytes);
    memcpy(&frame.frame_length, rx_buffer + parse_offset, sizeof(frame.frame_length));
    parse_offset += sizeof(frame.frame_length);
    memcpy(&frame.id, rx_buffer + parse_offset, sizeof(frame.id));
    parse_offset += sizeof(frame.id);
    memcpy(&frame.timestamp, rx_buffer + parse_offset, sizeof(frame.timestamp));
    parse_offset += sizeof(frame.timestamp);
    memcpy(&frame.status, rx_buffer + parse_offset, sizeof(frame.status));
    parse_offset += sizeof(frame.status);
    memcpy(&frame.flags, rx_buffer + parse_offset, sizeof(frame.flags));
    parse_offset += sizeof(frame.flags);
    memcpy(&frame.payload_size, rx_buffer + parse_offset, sizeof(frame.payload_size));
    parse_offset += sizeof(frame.payload_size);
    memcpy(frame.payload, rx_buffer + parse_offset, frame.payload_size);
    parse_offset += frame.payload_size;
    memcpy(&frame.crc32, rx_buffer + parse_offset, sizeof(frame.crc32));

    /* Push to ring (drop frame if ring is full) */
    if (!rx_ring_push(&frame))
    {
        /* Increase uart overrun count */
        increment_register_u16(REG_DEBUG_UART_OVERRUN_1);
    }
    else
    {
        increment_register_u32(REG_DEBUG_RX_PACKET_COUNT_1);
    }

    /* Reset state for next frame */
    rx_reset_state();
    return;
}

/**
 * @brief Build a response frame mirroring the request's id and flags.
 * @param response      Output frame to populate
 * @param request       Source request to mirror id/flags from
 * @param status        Operation status to return
 * @param payload       Optional payload data (may be NULL if size == 0)
 * @param payload_size  Size of the payload data
 */
static void build_response_frame(frame_t* response, const frame_t* request, status_t status, const void* payload, uint8_t payload_size)
{
    /* Check if response pointer is valid */
    if (response == NULL) return;

    /* Check if request is valid */
    if (request == NULL) return;

    /* Check if payload is valid */
    if ((payload == NULL) && payload_size) return;

    /* Clear response frame for safety */
    memset(response, 0x00, sizeof(frame_t));

    /* Mirror sync & ID from request, add response flag */
    response->sync_bytes = SYNC_BYTES;
    response->id         = request->id;
    response->timestamp  = 0;  /* will be filled by send_frame() */
    response->status     = status;
    response->flags      = request->flags | FLAG_MASK_RESPONSE;

    /* Payload */
    response->payload_size = payload_size;
    if ((payload != NULL) && (payload_size > 0))
    {
        memcpy(response->payload, payload, payload_size);
    }

    /* Frame length (Header + PayloadSize + Payload + CRC) */
    response->frame_length = HEADER_SIZE + sizeof(response->payload_size) + payload_size + CHECKSUM_SIZE;

    /* CRC will be calculated in send_frame() */
    response->crc32 = 0;

    return;
}

/**
 * @brief Handle a READ request: return the requested register bytes.
 * @param request   Incoming request frame
 * @param response  Outgoing response frame to populate
 *
 * Request payload format:  [start_addr: 1B] [length: 1B]
 * Response payload format: [start_addr: 1B] [length: 1B] [data: length B]
 */
static void handle_read(const frame_t* request, frame_t* response)
{
    /* Check if response is valid */
    if (response == NULL) return;

    /* Check if request is valid */
    if (request == NULL) return;

    /* Validate request payload size: [start_addr: 1B] [length: 1B] */
    if (request->payload_size != 2)
    {
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, NULL, 0);
        return;
    }

    uint8_t start_addr = request->payload[0];
    uint8_t length     = request->payload[1];

    /* Validate length & buffer space for response */
    if (!length || (((uint16_t)start_addr + length) > REGISTER_SIZE) || (length > (MAX_PAYLOAD_SIZE - 2)))
    {
        uint8_t err_payload[2] = { start_addr, length };
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, err_payload, 2);
        return;
    }

    /* Response payload: [start_addr: 1B] [length: 1B] [data: length B] */
    uint8_t response_payload[MAX_PAYLOAD_SIZE];
    response_payload[0] = start_addr;
    response_payload[1] = length;

    /* Read register data */
    if (!read_register(start_addr, response_payload + 2, length))
    {
        uint8_t err_payload[2] = { start_addr, length };
        build_response_frame(response, request, STATUS_ERR_INVALID_REG, err_payload, 2);
        return;
    }

    build_response_frame(response, request, STATUS_OK, response_payload, 2 + length);
    return;
}

/**
 * @brief Handle a WRITE request: write payload data to the register space.
 * @param request   Incoming request frame
 * @param response  Outgoing response frame to populate
 *
 * Request payload format:  [start_addr: 1B] [length: 1B] [data: length B]
 * Response payload format: [start_addr: 1B] [length: 1B]
 */
static void handle_write(const frame_t* request, frame_t* response)
{
    /* Check if response is valid */
    if (response == NULL) return;

    /* Check if request is valid */
    if (request == NULL) return;

    /* Validate request payload size: [start_addr: 1B] [length: 1B] [data: length B] */
    if (request->payload_size < 2)
    {
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, NULL, 0);
        return;
    }

    uint8_t start_addr = request->payload[0];
    uint8_t length     = request->payload[1];

    /* Validate length matches payload size */
    if (!length || (request->payload_size != (2 + length)))
    {
        uint8_t err_payload[2] = { start_addr, length };
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, err_payload, 2);
        return;
    }

    /* Validate address range */
    if (((uint16_t)start_addr + length) > REGISTER_SIZE)
    {
        uint8_t err_payload[2] = { start_addr, length };
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, err_payload, 2);
        return;
    }

    /* Write to register (with rights check, with callbacks) */
    if (!write_register(start_addr, request->payload + 2, length, REGISTER_CHECK_RIGHTS, REGISTER_ALLOW_CALLBACK))
    {
        uint8_t err_payload[2] = { start_addr, length };
        build_response_frame(response, request, STATUS_ERR_INVALID_REG, err_payload, 2);
        return;
    }

    /* ACK response: [start_addr: 1B] [length: 1B] */
    uint8_t ack_payload[2] = { start_addr, length };
    build_response_frame(response, request, STATUS_OK, ack_payload, 2);
    return;
}

/**
 * @brief Handle a PING request: return the current uptime as proof-of-life.
 * @param request   Incoming request frame
 * @param response  Outgoing response frame to populate
 *
 * Response payload format: [uptime_ms: 8B]
 */
static void handle_ping(const frame_t* request, frame_t* response)
{
    /* Check if response is valid */
    if (response == NULL) return;

    /* Check if request is valid */
    if (request == NULL) return;

    /* Read current uptime (8 byte from REG_TIME_UPTIME_1) */
    uint64_t uptime_ms = 0;
    if (!read_register(REG_TIME_UPTIME_1, &uptime_ms, sizeof(uptime_ms)))
    {
        build_response_frame(response, request, STATUS_ERR_GENERIC, NULL, 0);
        return;
    }

    /* Response payload: [uptime_ms: 8B] */
    build_response_frame(response, request, STATUS_OK, &uptime_ms, sizeof(uptime_ms));
    return;
}

/**
 * @brief Handle a TIME_SYNC request: adopt the host's UTC time.
 * @param request   Incoming request frame
 * @param response  Outgoing response frame to populate
 *
 * Request payload format:  [utc_ms: 8B]
 * Response payload format: empty (ACK)
 */
static void handle_time_sync(const frame_t* request, frame_t* response)
{
    /* Check if response is valid */
    if (response == NULL) return;

    /* Check if request is valid */
    if (request == NULL) return;

    /* Validate request payload: [utc_ms: 8B] */
    if (request->payload_size != sizeof(utc_timestamp_t))
    {
        build_response_frame(response, request, STATUS_ERR_INVALID_ARG, NULL, 0);
        return;
    }

    /* Extract UTC timestamp */
    utc_timestamp_t pc_utc_ms;
    memcpy(&pc_utc_ms, request->payload, sizeof(pc_utc_ms));

    /* Apply time sync */
    set_time_sync(pc_utc_ms);

    /* ACK response with empty payload */
    build_response_frame(response, request, STATUS_OK, NULL, 0);
    return;
}

/* Function definition */
crc32_t calc_crc32(const uint8_t* data, size_t length) 
{
    /* Check arguments */
    if ((data == NULL) || !length) return BASE_CRC32;

    /* Calculate crc32 */
    crc32_t crc = BASE_CRC32;
    for (size_t data_index = 0; data_index < length; data_index++) 
    {
        uint8_t table_index = (uint8_t)((crc ^ data[data_index]) & 0xFF);
        uint32_t entry = pgm_read_dword(&CRC32_TABLE[table_index]);
        crc = (crc >> 8) ^ entry;
    }
    crc = ~crc;

    return crc;
}

bool is_flag_valid(const flag_t flag)
{
    /* All valid masks (response flag can be matched with every flag so no check up for that) */
    static const flag_mask_t masks[] = 
    {
        FLAG_MASK_READ,
        FLAG_MASK_WRITE,
        FLAG_MASK_TIME_SYNC,
        FLAG_MASK_PING,
    };
    
    /* Search for flag bits, only one is allowed */
    bool flag_bit_found = false;
    for (uint8_t mask_index = 0; mask_index < (sizeof(masks) / sizeof(flag_mask_t)); mask_index++)
    {
        if (flag & masks[mask_index])
        {
            /* If one flag was found beforehand, flag is invalid */
            if (flag_bit_found) return FLAG_INVALID;

            /* First flag bit found */
            flag_bit_found = true;
        }
    }

    /* No flag bits found (invalid) */
    if (!flag_bit_found) return FLAG_INVALID;

    return FLAG_VALID;
}

void init_communication(void)
{
    Serial.begin(BAUD_RATE);
    rx_reset_state();
    rx_ring_head = 0;
    rx_ring_tail = 0;
    rx_ring_count = 0;
    return;
}

bool send_frame(const frame_t* frame)
{
    /* Check if frame is valid */
    if (frame == NULL) return false;

    /* Check if sync bytes are valid */
    if (frame->sync_bytes != SYNC_BYTES) return false;

    /* Check if frame length is valid */
    uint16_t expected_frame_length = HEADER_SIZE + sizeof(frame->payload_size) + frame->payload_size + CHECKSUM_SIZE;
    if (frame->frame_length != expected_frame_length) return false;

    /* Check if flag is valid */
    if (!is_flag_valid(frame->flags)) return false;

    /* Clear send buffer */
    memset(tx_buffer, 0x00, sizeof(tx_buffer));

    /* Get current utc time */
    utc_timestamp_t current_utc = 0;
    if (!read_register(REG_TIME_UTC_1, &current_utc, sizeof(current_utc))) return false;

    /* Preamble */
    uint16_t offset = 0;
    memcpy(tx_buffer + offset, &frame->sync_bytes, sizeof(frame->sync_bytes));
    offset += sizeof(frame->sync_bytes);
    memcpy(tx_buffer + offset, &frame->frame_length, sizeof(frame->frame_length));
    offset += sizeof(frame->frame_length);

    /* Header */
    memcpy(tx_buffer + offset, &frame->id, sizeof(frame->id));
    offset += sizeof(frame->id);
    memcpy(tx_buffer + offset, &current_utc, sizeof(current_utc));
    offset += sizeof(current_utc);
    memcpy(tx_buffer + offset, &frame->status, sizeof(frame->status));
    offset += sizeof(frame->status);
    memcpy(tx_buffer + offset, &frame->flags, sizeof(frame->flags));
    offset += sizeof(frame->flags);

    /* Payload */
    memcpy(tx_buffer + offset, &frame->payload_size, sizeof(frame->payload_size));
    offset += sizeof(frame->payload_size);
    memcpy(tx_buffer + offset, frame->payload, frame->payload_size);
    offset += frame->payload_size;

    /* Checksum */
    uint16_t crc_data_length = offset;
    crc32_t checksum = calc_crc32(tx_buffer, crc_data_length);
    memcpy(tx_buffer + offset, &checksum, sizeof(checksum));

    /* Send frame over serial */
    Serial.write(tx_buffer, PREAMBLE_SIZE + frame->frame_length);

    /* Increase transmit count */
    increment_register_u32(REG_DEBUG_TX_PACKET_COUNT_1); 

    return true;
}

void poll_serial(void)
{
    /* Check timeout if frame reception is in progress */
    if (rx_state != RX_WAIT_SYNC1)
    {
        uint64_t now = millis64();
        if ((now - rx_frame_start_uptime) > RX_TIMEOUT_MS)
        {
            /* Increase timeout count */
            increment_register_u16(REG_DEBUG_TIMEOUT_COUNT_1);

            rx_reset_state();
        }
    }

    /* Process all available bytes from serial */
    while (Serial.available() > 0)
    {
        uint8_t byte = (uint8_t)Serial.read();

        switch (rx_state)
        {
            case RX_WAIT_SYNC1:
                if (byte == 0xAA)
                {
                    rx_buffer[0] = byte;
                    rx_working_offset = 1;
                    rx_frame_start_uptime = millis64();
                    rx_state = RX_WAIT_SYNC2;
                }
                break;
            case RX_WAIT_SYNC2:
                if (byte == 0x55)
                {
                    rx_buffer[1] = byte;
                    rx_working_offset = 2;
                    rx_state = RX_WAIT_FRAME;
                }
                else if (byte == 0xAA)
                {
                    /* New sync attempt; keep first byte and wait for 0x55 */
                    rx_buffer[0] = byte;
                    rx_working_offset = 1;
                    rx_frame_start_uptime = millis64();
                }
                else
                {
                    /* Invalid sync, restart */
                    rx_reset_state();
                }
                break;
            case RX_WAIT_FRAME:
                /* Buffer overflow safeguard */
                if (rx_working_offset >= MAX_FRAME_SIZE)
                {
                    /* Increase generic error count */
                    increment_register_u16(REG_DEBUG_GEN_RX_ERR_COUNT_1);

                    rx_reset_state();

                    break;
                }

                /* Append byte to working buffer */
                rx_buffer[rx_working_offset++] = byte;

                /* Validate length */
                if (rx_working_offset == PREAMBLE_SIZE)
                {
                    uint16_t frame_length;
                    memcpy(&frame_length, rx_buffer + SYNC_BYTE_SIZE, sizeof(frame_length));

                    /* Sanity check: frame length must be plausible */
                    if ((frame_length < MIN_FRAME_LENGTH) || (frame_length > MAX_FRAME_LENGTH))
                    {
                        /* Increase generic error count */
                        increment_register_u16(REG_DEBUG_GEN_RX_ERR_COUNT_1);

                        rx_reset_state();
                        break;
                    }

                    rx_expected_length = PREAMBLE_SIZE + frame_length;
                }

                /* Validate frame */
                if (rx_expected_length && (rx_working_offset >= rx_expected_length))
                {
                    rx_validate_and_store();
                }
                break;
        }
    }

    return;
}

void process_frames(void)
{
    /* Process all available frames in ring */
    while (rx_ring_count)
    {
        /* Pop frame from ring */
        frame_t request;
        if (!rx_ring_pop(&request)) return;

        /* Register new frame as received for time register */
        new_frame_received_time();

        /* Validate flags before dispatch */
        if (!is_flag_valid(request.flags))
        {
            /* Build generic error response, mirror request id */
            frame_t response;
            build_response_frame(&response, &request, STATUS_ERR_GENERIC, NULL, 0);
            send_frame(&response);
            increment_register_u16(REG_DEBUG_GEN_RX_ERR_COUNT_1);
            record_error(response.status);
            continue;
        }

        /* Dispatch by flag type */
        frame_t response;
        if (request.flags & FLAG_MASK_READ)
        {
            handle_read(&request, &response);
        }
        else if (request.flags & FLAG_MASK_WRITE)
        {
            handle_write(&request, &response);
        }
        else if (request.flags & FLAG_MASK_PING)
        {
            handle_ping(&request, &response);
        }
        else if (request.flags & FLAG_MASK_TIME_SYNC)
        {
            handle_time_sync(&request, &response);
        }
        else
        {
            /* Should not reach here due to is_flag_valid check */
            build_response_frame(&response, &request, STATUS_ERR_GENERIC, NULL, 0);
        }

        /* Check for errors */
        if (response.status != STATUS_OK) record_error(response.status);

        /* Time sync in every frame */
        /* Extract UTC timestamp */
        //utc_timestamp_t pc_utc_ms;
        //memcpy(&pc_utc_ms, &request.timestamp, sizeof(pc_utc_ms));

        /* Apply time sync */
        //set_time_sync(pc_utc_ms);

        /* Send response */
        send_frame(&response);
    }

    return;
}