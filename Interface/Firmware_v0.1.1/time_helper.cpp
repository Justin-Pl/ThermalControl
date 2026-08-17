/**
 * @file    time_helper.cpp
 * @brief   Time tracking and PC synchronization implementation.
 *
 * Maintains a 64-bit millisecond counter on top of the AVR's 32-bit
 * millis(), tracks the offset to PC UTC time after a TIME_SYNC,
 * and updates the related register block on every call to
 * update_uptime().
 *
 * @author  Justin Plobst
 * @date    2026
 */

/* Header */
#include "time_helper.h"

/* Global variables */
/** @brief Last observed value of the 32-bit millis() counter. */
static uint32_t last_millis = 0;

/** @brief Accumulated 64-bit uptime in milliseconds. */
static uint64_t current_millis64 = 0;

/** @brief True once set_time_sync() has been called successfully. */
static bool time_synced = false;

/** @brief Uptime (ms) at which the most recent sync was applied. */
static uint64_t last_sync_uptime = 0;

/** @brief Uptime (ms) at which the most recent request frame arrived. */
static uint64_t last_request_uptime = 0;

/* Function definition */
uint64_t millis64(void)
{
    noInterrupts();

    /* Get current uint32 based millis */
    uint32_t current_millis = millis();

    /* Increment overflow save if time changed */
    if (last_millis != current_millis) 
    {
        current_millis64 += (uint32_t)(current_millis - last_millis);
        last_millis = current_millis;
    }

    interrupts();

    return current_millis64;
}

void set_time_sync(uint64_t pc_utc_ms)
{
    /* Get current uptime */
    uint64_t uptime = millis64();

    /* Calculate offset: utc = uptime + offset  =>  offset = utc - uptime */
    int64_t offset = (int64_t)pc_utc_ms - (int64_t)uptime;

    /* Store offset in register */
    if (!write_register(REG_TIME_OFFSET_1, &offset, sizeof(offset), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK)) return;

    /* Mark sync as valid & remember sync time */
    last_sync_uptime = uptime;
    time_synced = true;

    return;
}

void new_frame_received_time(void)
{
    last_request_uptime = millis64();
    return;
}

void update_uptime(void)
{
    /* Get current uptime & write to register */
    uint64_t current_uptime = millis64();
    write_register(REG_TIME_UPTIME_1, &current_uptime, sizeof(current_uptime), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

    /* If not yet synced with PC, nothing more to do */
    if (!time_synced) return;

    /* Read offset from register */
    int64_t offset = 0;
    if (!read_register(REG_TIME_OFFSET_1, &offset, sizeof(offset))) return;

    /* Calculate & write current UTC time */
    uint64_t current_utc = (uint64_t)((int64_t)current_uptime + offset);
    if (!write_register(REG_TIME_UTC_1, &current_utc, sizeof(current_utc), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK)) return;

    /* Calculate & write sync age */
    uint32_t sync_age = (uint32_t)(current_uptime - last_sync_uptime);
    write_register(REG_TIME_SYNC_AGE_1, &sync_age, sizeof(sync_age), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

    /* Calculate & write last request age */
    if (last_request_uptime)
    {
        uint32_t age = (uint32_t)(current_uptime - last_request_uptime);
        write_register(REG_DEBUG_LAST_REQ_AGE_1, &age, sizeof(age), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    }

    return;
}