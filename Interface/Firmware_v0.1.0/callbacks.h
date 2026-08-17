/**
 * @file    callbacks.h
 * @brief   Register read/write callback function declarations.
 *
 * These functions are invoked by the register subsystem when an
 * external client writes to (or reads from) specific registers,
 * triggering side effects such as a software reset, PWM update,
 * or recalculation of the calibrated temperature.
 *
 * @author  Justin Plobst
 * @date    2026
 */

#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

/* Libraries */
#include <Arduino.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>

/* Source files */
#include "register.h"
#include "communication.h"

/* Defines */
#define PWM_OUTPUT_PIN              8

/* Register write callbacks declaration */
/**
 * @brief Trigger a software reset if the magic byte was written.
 *
 * Validates that REG_SYS_RST_DEVICE contains REGISTER_RST_MAGIC_BYTE
 * before initiating the reset. Otherwise, the register is cleared
 * and the function returns. The reset is performed via the AVR
 * watchdog timer.
 *
 * @param addr  Start address of the triggering write
 * @param size  Number of bytes that were written
 */
void callback_reset_device(const uint8_t addr, const uint8_t size);

/**
 * @brief Clear all error counters if the magic byte was written.
 *
 * Validates that REG_SYS_CLEAR_ERRS contains REGISTER_CLR_ERR_MAGIC_BYTE
 * before clearing the system, input, and debug error counters.
 *
 * @param addr  Start address of the triggering write
 * @param size  Number of bytes that were written
 */
void callback_reset_errors(const uint8_t addr, const uint8_t size);

/**
 * @brief Apply a new PWM duty cycle to the MOSFET output.
 *
 * Reads REG_OUT_MOSFET_ENABLE and REG_OUT_MOSFET_PWM. If enable is
 * zero, the PWM output is forced to 0; otherwise the configured
 * PWM value is applied via analogWrite().
 *
 * @param addr  Start address of the triggering write
 * @param size  Number of bytes that were written
 */
void callback_change_pwm(const uint8_t addr, const uint8_t size);

/**
 * @brief Recalculate the calibrated temperature when calibration changes.
 *
 * Triggered on writes to REG_CAL_TEMP_OFFSET or REG_CAL_TEMP_GAIN.
 * Updates REG_IN_TEMP_CAL based on the latest raw reading and the
 * new calibration parameters, and updates REG_CAL_UTC_DATE with
 * the current UTC time.
 *
 * @param addr  Start address of the triggering write
 * @param size  Number of bytes that were written
 */
void callback_calibration_changed(const uint8_t addr, const uint8_t size);

/**
 * @brief Force an error for testing purposes.
 *
 * Reads the error code written to REG_TEST_FORCE_ERROR and records
 * it as the last error via record_error(). The register is cleared
 * after handling.
 *
 * @param addr  Start address of the triggering write
 * @param size  Number of bytes that were written
 */
void callback_force_error(const uint8_t addr, const uint8_t size);

/**
 * @brief Clear DATA_FRESH flag when temperature data is read.
 *
 * Triggered on reads of REG_IN_TEMP_RAW or REG_IN_TEMP_CAL. Sets
 * REG_IN_DATA_FRESH to 0 so the host can detect when a new
 * measurement has arrived.
 *
 * @param addr  Start address of the triggering read
 * @param size  Number of bytes that were read
 */
void callback_temp_read(const uint8_t addr, const uint8_t size);

#endif //_CALLBACKS_H_