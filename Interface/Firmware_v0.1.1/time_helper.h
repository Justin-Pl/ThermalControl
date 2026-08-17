/**
 * @file    time_helper.h
 * @brief   Time tracking and PC time synchronization.
 *
 * Provides a 64-bit millisecond uptime counter (built on top of the
 * AVR 32-bit millis()), a mechanism to synchronize with the host
 * PC's UTC clock, and periodic register updates for the Time
 * register block.
 *
 * Time synchronization works by storing an offset such that:
 *     UTC = uptime + offset
 *
 * The offset is derived from a TIME_SYNC frame sent by the host.
 *
 * @author  Justin Plobst
 * @date    2026
 */

#ifndef _TIME_HELPER_H_
#define _TIME_HELPER_H_

/* Libraries */
#include <Arduino.h>
#include <stdint.h>

/* Source files */
#include "register.h"

/* Function declaration */
/**
 * @brief Return uptime in milliseconds as a 64-bit value.
 *
 * Wraps the 32-bit AVR millis() and tracks overflows to provide
 * a monotonic 64-bit counter. Must be called regularly.
 *
 * @return Uptime in ms since boot
 */
uint64_t millis64(void);

/**
 * @brief Apply a UTC time synchronization from the host PC.
 *
 * Calculates the offset between PC UTC time and local uptime,
 * stores it in REG_TIME_OFFSET, and marks the sync as valid.
 *
 * @param pc_utc_ms  PC UTC time in milliseconds since 1970-01-01
 */
void set_time_sync(uint64_t pc_utc_ms);

/**
 * @brief Notify the time module that a new request frame has arrived.
 *
 * Used to compute REG_DEBUG_LAST_REQ_AGE for connection-watchdog
 * purposes. Called from process_frames() after a successful pop.
 */
void new_frame_received_time(void);

/**
 * @brief Refresh time-related registers from the current uptime.
 *
 * Updates REG_TIME_UPTIME unconditionally and (after a successful
 * sync) also REG_TIME_UTC, REG_TIME_SYNC_AGE, and
 * REG_DEBUG_LAST_REQ_AGE. Should be called regularly from the
 * main loop.
 */
void update_uptime(void);

#endif //_TIME_HELPER_H_