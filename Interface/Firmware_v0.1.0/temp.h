/**
 * @file    temp.h
 * @brief   DHT22 temperature sensor module.
 *
 * Provides asynchronous reading of a DHT22 sensor on a configured
 * digital pin. Sensor results are written to the input register
 * block (TEMP_RAW, TEMP_CAL, READING_AGE, DATA_FRESH, SENSOR_STATUS,
 * READ_COUNT, FAIL_COUNT).
 *
 * Calibrated temperature is computed as:
 *     TEMP_CAL = (TEMP_RAW * GAIN / 1000) + OFFSET
 *
 * @author  Justin Plobst
 * @date    2026
 */

#ifndef _TEMP_H_
#define _TEMP_H_

/* Libraries */
#include <Arduino.h>
#include <math.h>
#include <myDHTPro.h>

/* Source files */
#include "register.h"
#include "time_helper.h"

/* Defines */
/** @brief Digital pin connected to the DHT22 data line. */
#define DHT22_PIN                   2

/** @brief Max. theoretical temperature. */
#define TEMP_MAX_CELSIUS            327.67f

/** @brief Min. theoretical temperature. */
#define TEMP_MIN_CELSIUS            -327.68f

/* Function declaration */
/**
 * @brief Initialize the DHT22 sensor.
 *
 * Calls the underlying library's begin() and waits long enough
 * for the sensor to stabilize after power-on.
 */
void init_temp_sensor(void);

/**
 * @brief Drive the asynchronous sensor state machine.
 *
 * Should be called regularly from the main loop. Initiates a new
 * read if none is pending, processes any ongoing async I/O, and
 * updates the reading-age register.
 */
void update_temp_sensor(void);

#endif //_TEMP_H_