/**
 * @file    communication.h
 * @brief   Serial communication protocol definitions.
 *
 * Defines the binary frame format used between the host PC and the
 * firmware, the receive state machine, and the protocol-level types
 * (status codes, flag masks, frame structure).
 *
 * The protocol operates strictly synchronously in a request-response
 * pattern over UART. Multi-byte fields are little-endian.
 *
 * @author  Justin Plobst
 * @date    2026
 */

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

/* Libraries */
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Source files */
#include "time_helper.h"
#include "register.h"

/* Defines */
/** @brief UART baud rate used for host communication. */
#define BAUD_RATE                   115200

/** @brief Maximum payload size carried by one frame, in bytes. */
#define MAX_PAYLOAD_SIZE            255

/** @brief Number of sync bytes at the start of every frame. */
#define SYNC_BYTE_SIZE              2

/** @brief Size of the length field in the frame. */
#define LENGTH_SIZE                 2

/** @brief Size of the payload size field. */
#define PAYLOAD_SIZE_FIELD_SIZE     1

/** @brief Initial CRC32 register value (standard, polynomial 0xEDB88320). */
#define BASE_CRC32                  0xFFFFFFFF

/** @brief Return value of is_flag_valid() when the flag set is invalid. */
#define FLAG_INVALID                false

/** @brief Return value of is_flag_valid() when the flag set is valid. */
#define FLAG_VALID                  true

/** @brief Preamble size: sync bytes + frame length field. */
#define PREAMBLE_SIZE               (SYNC_BYTE_SIZE + LENGTH_SIZE)

/** @brief Header size: id + timestamp + status + flags. */
#define HEADER_SIZE                 (sizeof(frame_id_t) + sizeof(utc_timestamp_t) + sizeof(uint8_t) + sizeof(flag_t))

/** @brief Checksum size at the end of every frame. */
#define CHECKSUM_SIZE               sizeof(crc32_t)

/** @brief Minimum valid value of the frame_length field. */
#define MIN_FRAME_LENGTH            (HEADER_SIZE + PAYLOAD_SIZE_FIELD_SIZE + CHECKSUM_SIZE)

/** @brief Maximum valid value of the frame_length field. */
#define MAX_FRAME_LENGTH            (HEADER_SIZE + PAYLOAD_SIZE_FIELD_SIZE + MAX_PAYLOAD_SIZE + CHECKSUM_SIZE)

/** @brief Maximum total frame size including preamble. */
#define MAX_FRAME_SIZE              (PREAMBLE_SIZE + MAX_FRAME_LENGTH)

/** @brief Capacity of the receive ring buffer (number of frames). */
#define RX_RING_CAPACITY            4

/** @brief Frame reception timeout in milliseconds. */
#define RX_TIMEOUT_MS               200

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
    STATUS_OK              = 0x00,   /**< Operation succeeded               */
    STATUS_ERR_CRC         = 0x01,   /**< CRC mismatch (rarely sent)        */
    STATUS_ERR_INVALID_REG = 0x02,   /**< Invalid register or access type   */
    STATUS_ERR_INVALID_ARG = 0x03,   /**< Invalid payload format / length   */
    STATUS_ERR_GENERIC     = 0xFF    /**< Generic / unspecified error       */
} status_t;

/**
 * @brief Bit masks for the flags field of a frame.
 *
 * Exactly one operation flag (READ, WRITE, TIME_SYNC, PING) must be
 * set per frame. The RESPONSE bit is added to mark a frame as a reply.
 */
typedef enum : uint8_t
{
    FLAG_MASK_READ      = 0x01,   /**< Read register operation          */
    FLAG_MASK_WRITE     = 0x02,   /**< Write register operation         */
    FLAG_MASK_RESPONSE  = 0x04,   /**< Frame is a response, not request */
    FLAG_MASK_TIME_SYNC = 0x08,   /**< UTC time synchronization         */
    FLAG_MASK_PING      = 0x10    /**< Connectivity check               */
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

/* Function declaration */
/**
 * @brief Compute CRC32 (polynomial 0xEDB88320).
 * @param data    Pointer to data buffer
 * @param length  Number of bytes
 * @return CRC32 checksum, or BASE_CRC32 if input is invalid
 */
crc32_t calc_crc32(const uint8_t* data, size_t length);

/**
 * @brief Validate the flags field of a frame.
 *
 * Ensures exactly one operation flag (READ, WRITE, TIME_SYNC, PING)
 * is set. The RESPONSE bit is ignored.
 *
 * @param flag  Flags value from a frame
 * @return FLAG_VALID or FLAG_INVALID
 */
bool is_flag_valid(const flag_t flag);

/**
 * @brief Initialize the serial port and reset the receive state.
 *
 * Must be called once during setup before any other communication
 * function is used.
 */
void init_communication(void);

/**
 * @brief Serialize and send a frame over the UART.
 *
 * The CRC32 checksum and the timestamp are filled in automatically
 * before transmission. The frame_length field must be set correctly
 * by the caller (use build_response_frame() for that).
 *
 * @param frame  Pointer to a fully populated frame
 * @return true on success, false on validation or I/O failure
 */
bool send_frame(const frame_t* frame);

/**
 * @brief Drain the serial RX buffer and feed it to the parser.
 *
 * Should be called regularly in the main loop. Validated frames are
 * pushed to an internal ring buffer for later processing by
 * process_frames().
 */
void poll_serial(void);

/**
 * @brief Dispatch and respond to all queued frames in the RX ring.
 *
 * Pops every available frame, executes the matching handler, and
 * sends a response back. Should be called regularly in the main loop.
 */
void process_frames(void);

#endif //_COMMUNICATION_H_