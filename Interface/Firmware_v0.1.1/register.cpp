/**
 * @file    register.cpp
 * @brief   Register address space implementation.
 *
 * Implements the 256-byte register storage, access-rights bitmaps, and
 * the read/write API. Register access can optionally trigger registered
 * read or write callbacks for register-specific side effects (e.g. PWM
 * update, device reset).
 *
 * @author  Justin Plobst
 * @date    2026
 */

/* Header */
#include "register.h"

/* Type definitions */
/* Structs */
/**
 * @brief Handler entry mapping a register range to a write callback.
 */
typedef struct
{
    uint8_t addr_start;             /**< First address covered by handler */
    uint8_t addr_end;               /**< Last address covered by handler  */
    reg_write_callback_t callback;  /**< Function to invoke on write      */
} reg_write_handler_t;

/**
 * @brief Handler entry mapping a register range to a read callback.
 */
typedef struct
{
    uint8_t addr_start;             /**< First address covered by handler */
    uint8_t addr_end;               /**< Last address covered by handler  */
    reg_read_callback_t callback;   /**< Function to invoke on read       */
} reg_read_handler_t;

/* Constant variables */
/**
 * @brief Write callback dispatch table.
 *
 * Maps register address ranges to callback functions that are invoked
 * when a write occurs to any byte within the range. Stored in PROGMEM
 * to save SRAM.
 */
static const reg_write_handler_t REG_WRITE_HANDLERS[] PROGMEM = 
{
    { REG_SYS_RST_DEVICE,    REG_SYS_RST_DEVICE,    callback_reset_device },
    { REG_SYS_CLEAR_ERRS,    REG_SYS_CLEAR_ERRS,    callback_reset_errors },
    { REG_OUT_MOSFET_PWM,    REG_OUT_MOSFET_PWM,    callback_change_pwm },
    { REG_OUT_MOSFET_ENABLE, REG_OUT_MOSFET_ENABLE, callback_change_pwm },
    { REG_CONFIG_SAVE,       REG_CONFIG_SAVE,       NULL },
    { REG_CAL_TEMP_OFFSET_1, REG_CAL_TEMP_GAIN_2, callback_calibration_changed },
    { REG_TEST_FORCE_ERROR, REG_TEST_FORCE_ERROR, callback_force_error },
};
/** @brief Number of entries in the write callback dispatch table. */
static const uint8_t REG_WRITE_HANDLERS_COUNT = sizeof(REG_WRITE_HANDLERS) / sizeof(reg_write_handler_t);

/**
 * @brief Read callback dispatch table.
 *
 * Maps register address ranges to callback functions that are invoked
 * after a read operation completes. Used for example to clear the
 * DATA_FRESH flag once temperature data has been retrieved.
 */
static const reg_read_handler_t REG_READ_HANDLERS[] PROGMEM = 
{
    { REG_IN_TEMP_RAW_1, REG_IN_TEMP_RAW_2, callback_temp_read },
    { REG_IN_TEMP_CAL_1, REG_IN_TEMP_CAL_2, callback_temp_read },
};

/** @brief Number of entries in the read callback dispatch table. */
static const uint8_t REG_READ_HANDLERS_COUNT = sizeof(REG_READ_HANDLERS) / sizeof(reg_read_handler_t);

/* Global variables */
/** @brief Backing storage for the 256-byte register address space. */
static uint8_t register_data[REGISTER_SIZE];

/**
 * @brief Per-byte bitmask of which bits are functionally used.
 *
 * Bits set to 1 are considered valid; bits set to 0 are reserved and
 * are forced to 0 on every read/write via bitwise AND.
 */
static uint8_t register_valid_bits[REGISTER_SIZE];

/**
 * @brief Read-permission bitmap (1 bit per register address).
 *
 * Bit (addr / 8) of byte (addr % 8) indicates whether external clients
 * may read the corresponding register byte.
 */
static uint8_t register_readable_bitmap[REGISTER_SIZE / 8];

/**
 * @brief Write-permission bitmap (1 bit per register address).
 *
 * Bit (addr / 8) of byte (addr % 8) indicates whether external clients
 * may write the corresponding register byte.
 */
static uint8_t register_writeable_bitmap[REGISTER_SIZE / 8];

/* Inline functions */
/**
 * @brief Test whether the given register address is readable.
 * @param addr  Register address (0x00 - 0xFF)
 * @return true if readable, false otherwise
 */
static inline bool is_register_readable(uint8_t addr) 
{
    return register_readable_bitmap[addr / 8] & (1 << (addr & 7));
}

/**
 * @brief Test whether the given register address is writeable.
 * @param addr  Register address (0x00 - 0xFF)
 * @return true if writeable, false otherwise
 */
static inline bool is_register_writeable(uint8_t addr) 
{
    return register_writeable_bitmap[addr / 8] & (1 << (addr & 7));
}

/**
 * @brief Mark the given register address as readable.
 * @param addr  Register address (0x00 - 0xFF)
 */
static inline void set_register_readable(uint8_t addr) 
{
    register_readable_bitmap[addr / 8] |= (1 << (addr & 7));
}

/**
 * @brief Mark the given register address as writeable.
 * @param addr  Register address (0x00 - 0xFF)
 */
static inline void set_register_writeable(uint8_t addr) 
{
    register_writeable_bitmap[addr / 8] |= (1 << (addr & 7));
}

/* Function definition */
void init_registers(void)
{
    /* Init register data & register rights */
    memset(register_data,             0xFF, REGISTER_SIZE);
    memset(register_valid_bits,       0x00, REGISTER_SIZE);
    memset(register_readable_bitmap,  0x00, REGISTER_SIZE / 8);
    memset(register_writeable_bitmap, 0x00, REGISTER_SIZE / 8);

    /* Define register rights */
    /* System (0x00 - 0x1F) */
    define_register_rights(REG_SYS_FW_VERSION_MAJOR, 1, REG_READ_ONLY);
    define_register_rights(REG_SYS_FW_VERSION_MINOR, 1, REG_READ_ONLY);
    define_register_rights(REG_SYS_FW_VERSION_PATCH, 1, REG_READ_ONLY);
    define_register_rights(REG_SYS_RST_DEVICE,       1, REG_WRITE_ONLY);
    define_register_rights(REG_SYS_LAST_ERR_CODE,    1, REG_READ_ONLY);
    define_register_rights(REG_SYS_CLEAR_ERRS,       1, REG_WRITE_ONLY);
    define_register_rights(REG_SYS_ERR_COUNT_1,      4, REG_READ_ONLY);

    /* Time (0x20 - 0x3F) */
    define_register_rights(REG_TIME_UPTIME_1,        8, REG_READ_ONLY);
    define_register_rights(REG_TIME_UTC_1,           8, REG_READ_ONLY);
    define_register_rights(REG_TIME_OFFSET_1,        8, REG_READ_ONLY);
    define_register_rights(REG_TIME_SYNC_AGE_1,      4, REG_READ_ONLY);

    /* Input - Sensor (0x40 - 0x5F) */
    define_register_rights(REG_IN_TEMP_RAW_1,        2, REG_READ_ONLY);
    define_register_rights(REG_IN_TEMP_CAL_1,        2, REG_READ_ONLY);
    define_register_rights(REG_IN_READING_AGE_1,     4, REG_READ_ONLY);
    define_register_rights(REG_IN_DATA_FRESH,        1, REG_READ_ONLY);
    define_register_rights(REG_IN_SENSOR_STATUS,     1, REG_READ_ONLY);
    define_register_rights(REG_IN_READ_COUNT_1,      4, REG_READ_ONLY);
    define_register_rights(REG_IN_FAIL_COUNT_1,      4, REG_READ_ONLY);

    /* Output - Aktor (0x60 - 0x6F) */
    define_register_rights(REG_OUT_MOSFET_PWM,       1, REG_READ_WRITE);
    define_register_rights(REG_OUT_MOSFET_ENABLE,    1, REG_READ_WRITE);
    /* Init pwm output for mosfet */
    pinMode(PWM_OUTPUT_PIN, OUTPUT);
    analogWrite(PWM_OUTPUT_PIN, 0);

    /* Config (0x70 - 0x7F) */
    define_register_rights(REG_CONFIG_SAVE,          1, REG_WRITE_ONLY);
    define_register_rights(REG_CONFIG_LOAD_DEFAULTS, 1, REG_WRITE_ONLY);

    /* Calibration (0x80 - 0x9F) */
    define_register_rights(REG_CAL_TEMP_OFFSET_1,    2, REG_READ_WRITE);
    define_register_rights(REG_CAL_TEMP_GAIN_1,      2, REG_READ_WRITE);
    define_register_rights(REG_CAL_UTC_DATE_1,       8, REG_READ_ONLY);

    /* Debug (0xA0 - 0xBF) */
    define_register_rights(REG_DEBUG_CRC_ERR_COUNT_1,     2, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_TIMEOUT_COUNT_1,     2, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_UART_OVERRUN_1,      2, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_GEN_RX_ERR_COUNT_1,  2, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_RX_PACKET_COUNT_1,   4, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_TX_PACKET_COUNT_1,   4, REG_READ_ONLY);
    define_register_rights(REG_DEBUG_LAST_REQ_AGE_1,      4, REG_READ_ONLY);

    /* Test (0xF0 - 0xFF) */
    define_register_rights(REG_TEST_FORCE_ERROR,     1, REG_WRITE_ONLY);

    /* Define default value for register & which bits are valid bits */
    /* System */
    default_register_value(REG_SYS_FW_VERSION_MAJOR, FW_VERSION_MAJOR, 0xFF);
    default_register_value(REG_SYS_FW_VERSION_MINOR, FW_VERSION_MINOR, 0xFF);
    default_register_value(REG_SYS_FW_VERSION_PATCH, FW_VERSION_PATCH, 0xFF);
    default_register_value(REG_SYS_RST_DEVICE,       0x00,             0xFF);
    default_register_value(REG_SYS_LAST_ERR_CODE,    STATUS_OK,        0xFF);
    default_register_value(REG_SYS_CLEAR_ERRS,       0x00,             0xFF);
    default_register_value(REG_SYS_ERR_COUNT_1,      0x00,             0xFF);
    default_register_value(REG_SYS_ERR_COUNT_2,      0x00,             0xFF);
    default_register_value(REG_SYS_ERR_COUNT_3,      0x00,             0xFF);
    default_register_value(REG_SYS_ERR_COUNT_4,      0x00,             0xFF);

    /* Time */
    for (uint8_t index = 0; index < 8; index++) default_register_value(REG_TIME_UPTIME_1   + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 8; index++) default_register_value(REG_TIME_UTC_1      + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 8; index++) default_register_value(REG_TIME_OFFSET_1   + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_TIME_SYNC_AGE_1 + index, 0x00, 0xFF);

    /* Input */
    default_register_value(REG_IN_TEMP_RAW_1,        0x00, 0xFF);
    default_register_value(REG_IN_TEMP_RAW_2,        0x00, 0xFF);
    default_register_value(REG_IN_TEMP_CAL_1,        0x00, 0xFF);
    default_register_value(REG_IN_TEMP_CAL_2,        0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_IN_READING_AGE_1 + index, 0x00, 0xFF);
    default_register_value(REG_IN_DATA_FRESH,        0x00, 0x01);
    default_register_value(REG_IN_SENSOR_STATUS,     0x00, 0x7F);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_IN_READ_COUNT_1  + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_IN_FAIL_COUNT_1  + index, 0x00, 0xFF);

    /* Output */
    default_register_value(REG_OUT_MOSFET_PWM,       0x00, 0xFF);
    default_register_value(REG_OUT_MOSFET_ENABLE,    0x00, 0x01);

    /* Config */
    default_register_value(REG_CONFIG_SAVE,          0x00, 0x01);
    default_register_value(REG_CONFIG_LOAD_DEFAULTS, 0x00, 0x01);

    /* Calibration */
    default_register_value(REG_CAL_TEMP_OFFSET_1,    0x00, 0xFF);
    default_register_value(REG_CAL_TEMP_OFFSET_2,    0x00, 0xFF);
    default_register_value(REG_CAL_TEMP_GAIN_1,      0xE8, 0xFF);  /* Default Gain = 1000 (0x03E8) */
    default_register_value(REG_CAL_TEMP_GAIN_2,      0x03, 0xFF);
    for (uint8_t index = 0; index < 8; index++) default_register_value(REG_CAL_UTC_DATE_1 + index, 0x00, 0xFF);

    /* Debug */
    for (uint8_t index = 0; index < 2; index++) default_register_value(REG_DEBUG_CRC_ERR_COUNT_1    + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 2; index++) default_register_value(REG_DEBUG_TIMEOUT_COUNT_1    + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 2; index++) default_register_value(REG_DEBUG_UART_OVERRUN_1     + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 2; index++) default_register_value(REG_DEBUG_GEN_RX_ERR_COUNT_1 + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_DEBUG_RX_PACKET_COUNT_1  + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_DEBUG_TX_PACKET_COUNT_1  + index, 0x00, 0xFF);
    for (uint8_t index = 0; index < 4; index++) default_register_value(REG_DEBUG_LAST_REQ_AGE_1     + index, 0x00, 0xFF);

    /* Test */
    default_register_value(REG_TEST_FORCE_ERROR,     0x00, 0xFF);

    return;
}

void define_register_rights(const register_t addr, const uint8_t size, const register_right_t rights)
{
    /* Size must be atleast one */
    if (!size) return;

    /* Iterate through every byte of the register & define rights */
    for (uint16_t addr_index = 0; addr_index < size; addr_index++)
    {
        /* Check if address is out of bounds */
        if (((uint16_t)addr + addr_index) >= REGISTER_SIZE) break;

        /* Set register rights */
        switch (rights)
        {
            case REG_READ_ONLY:
                set_register_readable(addr + addr_index);
                break;
            case REG_WRITE_ONLY:
                set_register_writeable(addr + addr_index);
                break;
            case REG_READ_WRITE:
                set_register_readable(addr + addr_index);
                set_register_writeable(addr + addr_index);
                break;
        }
    }

    return;
}

void default_register_value(const register_t addr, const uint8_t value, const uint8_t valid_bits)
{
    register_data[addr] = value;
    register_valid_bits[addr] = valid_bits;
    return;
}

bool read_register(const register_t addr, void* data, const uint8_t size)
{
    /* Check if address & range is valid */
    if (!size || (((uint16_t)addr + size) > REGISTER_SIZE)) return false;

    /* Check if data is valid */
    if (data == NULL) return false;

    /* Check for right violation */
    for (uint16_t addr_index = addr; addr_index < ((uint16_t)addr + size); addr_index++)
    {
        if (!is_register_readable((uint8_t)addr_index)) return false;
    }

    /* Read atomar from register */
    noInterrupts();
    for (uint8_t byte_index = 0; byte_index < size; byte_index++) 
    {
        ((uint8_t*)data)[byte_index] = register_data[addr + byte_index] & register_valid_bits[addr + byte_index];
    }
    interrupts();

    /* Callback for register actions */
    uint16_t read_end = (uint16_t)addr + size - 1;
    for (uint8_t handler_index = 0; handler_index < REG_READ_HANDLERS_COUNT; handler_index++)
    {
        /* Get start & end address of handler */
        uint8_t handler_start = pgm_read_byte(&REG_READ_HANDLERS[handler_index].addr_start);
        uint8_t handler_end   = pgm_read_byte(&REG_READ_HANDLERS[handler_index].addr_end);

        /* Check if handler is for adress room */
        if (addr > handler_end) continue;
        if (read_end < handler_start) continue;

        /* Call handler */
        reg_read_callback_t callback = (reg_read_callback_t)pgm_read_ptr(&REG_READ_HANDLERS[handler_index].callback);
        if (callback != NULL) callback(addr, size);
    }

    return true;
}

bool write_register(const register_t addr, const void* data, const uint8_t size, const bool force, const bool trigger_callback)
{
    /* Check if address & range is valid */
    if (!size || (((uint16_t)addr + size) > REGISTER_SIZE)) return false;

    /* Check if data is valid */
    if (data == NULL) return false;

    /* Check for right violation */
    if (!force)
    {
        for (uint8_t addr_index = addr; addr_index < ((uint16_t)addr + size); addr_index++)
        {
            if (!is_register_writeable(addr_index)) return false;
        }
    }

    /* Write atomar to register */
    noInterrupts();
    for (uint8_t byte_index = 0; byte_index < size; byte_index++) 
    {
        uint8_t byte_data = ((const uint8_t*)data)[byte_index];
        register_data[addr + byte_index] = byte_data & register_valid_bits[addr + byte_index];
    }
    interrupts();

    /* If no callback return early */
    if (!trigger_callback) return true;

    /* Callback for register actions */
    uint16_t write_end = (uint16_t)addr + size - 1;
    for (uint8_t handler_index = 0; handler_index < REG_WRITE_HANDLERS_COUNT; handler_index++)
    {
        /* Get start & end address of handler */
        uint8_t handler_start = pgm_read_byte(&REG_WRITE_HANDLERS[handler_index].addr_start);
        uint8_t handler_end   = pgm_read_byte(&REG_WRITE_HANDLERS[handler_index].addr_end);

        /* Check if handler is for adress room */
        if (addr > handler_end) continue;
        if (write_end < handler_start) continue;

        /* Call handler */
        reg_write_callback_t callback = (reg_write_callback_t)pgm_read_ptr(&REG_WRITE_HANDLERS[handler_index].callback);
        if (callback != NULL) callback(addr, size);
    }

    return true;
}

void increment_register_u16(const register_t reg_addr)
{
    uint16_t count = 0;
    if (!read_register(reg_addr, &count, sizeof(count))) return;
    count++;
    write_register(reg_addr, &count, sizeof(count), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    return;
}

void increment_register_u32(const register_t reg_addr)
{
    uint32_t count = 0;
    if (!read_register(reg_addr, &count, sizeof(count))) return;
    count++;
    write_register(reg_addr, &count, sizeof(count), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    return;
}

void increment_register_u64(const register_t reg_addr)
{
    uint64_t count = 0;
    if (!read_register(reg_addr, &count, sizeof(count))) return;
    count++;
    write_register(reg_addr, &count, sizeof(count), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    return;
}

void record_error(const uint8_t code)
{
    /* Set last error */
    write_register(REG_SYS_LAST_ERR_CODE, &code, sizeof(code), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    
    /* Increment error count */
    increment_register_u32(REG_SYS_ERR_COUNT_1);

    return;
}