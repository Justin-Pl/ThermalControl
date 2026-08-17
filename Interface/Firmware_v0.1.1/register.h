/**
 * @file    register.h
 * @brief   Register address space definition and access functions.
 *
 * The interface uses a 256-byte register address space as a unified
 * data and control surface between the host PC and the firmware.
 * Each byte has its own access rights and bit mask.
 *
 * @author  Justin Plobst
 * @date    2026
 */

#ifndef _REGISTER_H_
#define _REGISTER_H_

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Source files */
#include "callbacks.h"

/* Defines */
/** @brief Firmware version major number */
#define FW_VERSION_MAJOR             0

/** @brief Firmware version minor number */
#define FW_VERSION_MINOR             1

/** @brief Firmware version patch number */
#define FW_VERSION_PATCH             1

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

/* Magic bytes */
/** @brief Magic byte that triggers a software reset when written to RST_DEVICE */
#define REGISTER_RST_MAGIC_BYTE      0xA5

/** @brief Magic byte that triggers an error counter clear when written to CLEAR_ERRS */
#define REGISTER_CLR_ERR_MAGIC_BYTE  0xA5

/* Register access modifiers */
/** @brief Bypass access-rights check on write */
#define REGISTER_FORCE_WRITE         true

/** @brief Enforce access-rights check on write */
#define REGISTER_CHECK_RIGHTS        false

/** @brief Skip write callback dispatch */
#define REGISTER_NO_CALLBACK         false

/** @brief Run write callback dispatch */
#define REGISTER_ALLOW_CALLBACK      true

/* Function pointers */
/**
 * @brief Callback type for register write events.
 * @param addr  Start address of the write operation
 * @param size  Number of bytes written
 */
typedef void (*reg_write_callback_t)(const uint8_t addr, uint8_t size);

/**
 * @brief Callback type for register read events.
 * @param addr  Start address of the read operation
 * @param size  Number of bytes read
 */
typedef void (*reg_read_callback_t)(const uint8_t addr, uint8_t size);

/* Enumarations */
/**
 * @brief Access rights for a register byte.
 */
typedef enum
{
    REG_READ_ONLY,    /**< Register can only be read by external clients  */
    REG_WRITE_ONLY,   /**< Register can only be written by external clients */
    REG_READ_WRITE    /**< Register can be read and written                */
} register_right_t;


/**
 * @brief Logical register addresses in the 256-byte register space.
 *
 * Each enum value corresponds to one byte address. Multi-byte values
 * occupy multiple consecutive enum entries (e.g. UPTIME_1..UPTIME_8 for
 * a 64-bit value, stored in little-endian order).
 */
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

/* Function declaration */
/**
 * @brief Initialize register data, valid bits, and access rights.
 *
 * Sets all register data to 0xFF, clears bitmaps, and applies the
 * default register definitions for every used register address.
 */
void init_registers(void);

/**
 * @brief Configure access rights for a contiguous register range.
 * @param addr    Starting address
 * @param size    Number of bytes
 * @param rights  Access rights to apply to all bytes in the range
 */
void define_register_rights(const register_t addr, const uint8_t size, const register_right_t rights);

/**
 * @brief Set the default value and valid-bits mask for a single byte.
 * @param addr        Register address
 * @param value       Initial value
 * @param valid_bits  Bitmask of bits actually used by this register
 */
void default_register_value(const register_t addr, const uint8_t value, const uint8_t valid_bits);

/**
 * @brief Read one or more consecutive register bytes.
 * @param addr  Start address
 * @param data  Destination buffer (must be at least @p size bytes)
 * @param size  Number of bytes to read
 * @return true on success, false on invalid address, size, or rights
 */
bool read_register(const register_t addr, void* data, const uint8_t size);

/**
 * @brief Write one or more consecutive register bytes.
 * @param addr              Start address
 * @param data              Source buffer
 * @param size              Number of bytes to write
 * @param force             If true, skip access-rights check
 * @param trigger_callback  If true, dispatch matching write callbacks
 * @return true on success, false on invalid address, size, or rights
 */
bool write_register(const register_t addr, const void* data, const uint8_t size, const bool force, const bool trigger_callback);

/**
 * @brief Record an error: store the last error code and increment count.
 * @param code  Error code (typically a status_t value)
 */
void record_error(const uint8_t code);

/**
 * @brief Atomically increment a 16-bit counter register.
 * @param reg_addr  Register address of the counter (LSB)
 */
void increment_register_u16(const register_t reg_addr);

/**
 * @brief Atomically increment a 32-bit counter register.
 * @param reg_addr  Register address of the counter (LSB)
 */
void increment_register_u32(const register_t reg_addr);

/**
 * @brief Atomically increment a 64-bit counter register.
 * @param reg_addr  Register address of the counter (LSB)
 */
void increment_register_u64(const register_t reg_addr);

#endif //_REGISTER_H_