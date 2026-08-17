/**
 * @file    callbacks.cpp
 * @brief   Implementation of register write/read callbacks.
 *
 * Each callback is invoked by the register subsystem when a specific
 * register (or range) is written to or read from. Callbacks
 * encapsulate side effects of register access:
 *
 * - callback_reset_device         : software reset via watchdog
 * - callback_reset_errors         : clear all error counters
 * - callback_change_pwm           : update MOSFET PWM output
 * - callback_temp_read            : clear DATA_FRESH after read
 * - callback_calibration_changed  : update calibration date and recalc
 * - callback_force_error          : inject an error for testing
 *
 * Callbacks always use REGISTER_FORCE_WRITE and REGISTER_NO_CALLBACK
 * for internal writes to avoid recursion and bypass access-rights
 * checks for system-internal updates.
 *
 * @author  Justin Plobst
 * @date    2026
 */

/* Header */
#include "callbacks.h"

/* Register write callbacks definition */
void callback_reset_device(const uint8_t addr, uint8_t size)
{
    /* Check for magic byte for resetting */
    uint8_t reset_reg;
    if (!read_register(REG_SYS_RST_DEVICE, &reset_reg, sizeof(reset_reg))) return;
    if (reset_reg != REGISTER_RST_MAGIC_BYTE)
    {
        reset_reg = 0x00;
        write_register(REG_SYS_RST_DEVICE, &reset_reg, sizeof(reset_reg), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
        return;
    }

    /* AVR reset via watchdog */
    wdt_enable(WDTO_15MS);
    while (1);

    return;
}

void callback_reset_errors(const uint8_t addr, uint8_t size)
{
    /* Check for magic byte for resetting errors */
    uint8_t reset_reg;
    if (!read_register(REG_SYS_CLEAR_ERRS, &reset_reg, sizeof(reset_reg))) return;
    if (reset_reg != REGISTER_CLR_ERR_MAGIC_BYTE)
    {
        reset_reg = 0x00;
        write_register(REG_SYS_CLEAR_ERRS, &reset_reg, sizeof(reset_reg), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
        return;
    }

    /* Clear all error counters */
    uint32_t empty_dummy = 0x00000000;
    /* System */
    write_register(REG_SYS_LAST_ERR_CODE, &empty_dummy, 1, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    write_register(REG_SYS_ERR_COUNT_1, &empty_dummy, 4, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    /* Input */
    write_register(REG_IN_FAIL_COUNT_1, &empty_dummy, 4, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    /* Debug */
    write_register(REG_DEBUG_CRC_ERR_COUNT_1, &empty_dummy, 2, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    write_register(REG_DEBUG_TIMEOUT_COUNT_1, &empty_dummy, 2, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    write_register(REG_DEBUG_UART_OVERRUN_1, &empty_dummy, 2, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    write_register(REG_DEBUG_GEN_RX_ERR_COUNT_1, &empty_dummy, 2, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

    return;
}

void callback_change_pwm(const uint8_t addr, uint8_t size)
{
    /* Change pwm duty if pwm is enabled & according to pwm register */
    uint8_t pwm_enable_reg;
    if (!read_register(REG_OUT_MOSFET_ENABLE, &pwm_enable_reg, sizeof(pwm_enable_reg))) 
    {
        analogWrite(PWM_OUTPUT_PIN, 0);
        return;
    }
    if (pwm_enable_reg)
    {
        uint8_t pwm_reg;
        if (!read_register(REG_OUT_MOSFET_PWM, &pwm_reg, sizeof(pwm_reg))) return;
        analogWrite(PWM_OUTPUT_PIN, pwm_reg);
        return;
    }
    else
    {
        analogWrite(PWM_OUTPUT_PIN, 0);
    }

    return;
}

void callback_temp_read(const uint8_t addr, uint8_t size)
{
    /* Reset data ready bit */
    uint8_t empty_dummy = 0x00;
    write_register(REG_IN_DATA_FRESH, &empty_dummy, 1, REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    return;
}

void callback_calibration_changed(const uint8_t addr, uint8_t size)
{
    /* Update calibration date with current UTC */
    utc_timestamp_t now = 0;
    read_register(REG_TIME_UTC_1, &now, sizeof(now));
    write_register(REG_CAL_UTC_DATE_1, &now, sizeof(now), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    
    /* Recalculate TEMP_CAL with new calibration values */
    int16_t temp_raw = 0;
    int16_t offset = 0;
    int16_t gain = 1000;
    read_register(REG_IN_TEMP_RAW_1, &temp_raw, sizeof(temp_raw));
    read_register(REG_CAL_TEMP_OFFSET_1, &offset, sizeof(offset));
    read_register(REG_CAL_TEMP_GAIN_1, &gain, sizeof(gain));
    if (gain == 0) gain = 1000;
    
    int32_t temp_cal_32 = ((int32_t)temp_raw * gain) / 1000 + offset;
    if (temp_cal_32 >  32767) temp_cal_32 =  32767;
    if (temp_cal_32 < -32768) temp_cal_32 = -32768;
    int16_t temp_cal = (int16_t)temp_cal_32;
    
    write_register(REG_IN_TEMP_CAL_1, &temp_cal, sizeof(temp_cal), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    return;
}

void callback_force_error(const uint8_t addr, uint8_t size)
{
    /* Get custom forced error code from force error reg */
    uint8_t err_code = 0;
    read_register(REG_TEST_FORCE_ERROR, &err_code, sizeof(err_code));
    
    /* Record error */
    record_error(err_code);
    
    /* Clear force error reg again */
    uint8_t dummy = 0x00;
    write_register(REG_TEST_FORCE_ERROR, &dummy, sizeof(dummy), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

    return;
}