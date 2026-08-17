/**
 * @file    temp.cpp
 * @brief   DHT22 temperature sensor implementation.
 *
 * Wraps the MyDHTPro async API and writes sensor readings (raw and
 * calibrated) to the register space. On read failure, the sensor
 * status register reflects the error condition and the fail counter
 * is incremented.
 *
 * Calibration is applied on every successful reading using the
 * current values of REG_CAL_TEMP_OFFSET and REG_CAL_TEMP_GAIN.
 *
 * @author  Justin Plobst
 * @date    2026
 */

/* Header */
#include "temp.h"

/* Global variables */
/** @brief DHT22 driver instance bound to DHT22_PIN. */
static MyDHT temp_sensor(DHT22_PIN);

/** @brief Uptime (ms) of the last successful temperature reading. */
static uint64_t last_successful_read_uptime = 0;

/** @brief When to call next callback. */
static uint64_t next_callback_time = 0;

/** @brief Whether at least one successful reading has been recorded. */
static bool has_successful_read = false;

/** @brief Number of pin transitions expected for one full DHT frame (2 ACK + 40*2 bit edges + 1 trailing edge). */
static const uint8_t DHT_EDGE_COUNT = 83;

/** @brief Timestamps (µs) of pin transitions captured by the edge ISR. */
static volatile uint32_t edgeTs[DHT_EDGE_COUNT];

/** @brief Number of edges captured in the current frame. */
static volatile uint8_t edgeIdx = 0;

/** @brief Whether the edge ISR is currently allowed to record timestamps. */
static volatile bool captureActive = false;

/** @brief States of the custom, interrupt-driven DHT capture state machine. */
enum DhtCaptureState
{
    DHT_IDLE,
    DHT_START_LOW,
    DHT_WAIT_FRAME
};

/** @brief Current state of the DHT capture state machine. */
static DhtCaptureState dhtState = DHT_IDLE;

/** @brief Timer (ms or µs, depending on state) used to advance the capture state machine. */
static uint32_t dhtStateTimer = 0;

/** @brief Timestamp when signal led was light up */
static uint64_t led_timestamp = 0;

/* Static function definition */
/**
 * @brief Callback invoked by the DHT library when a read completes.
 *
 * On success: writes raw and calibrated temperature, resets reading
 * age, marks data as fresh, updates sensor status, and increments
 * the read counter.
 *
 * On failure: writes the failure mode to the sensor status bitfield
 * and increments the fail counter.
 *
 * @param data  Result struct from the DHT library
 */
static void temp_sensor_callback(DHTData data)
{
    next_callback_time = millis64() + temp_sensor.getMinReadInterval();
    if (data.status == DHT_OK)
    {
        /* Light up signal led */
        digitalWrite(LED_BUILTIN, HIGH);
        led_timestamp = millis64();

        /* Convert float temperature to int16 with factor 100 (°C × 100) */
        float temp_clamped = data.temp;
        if (temp_clamped > TEMP_MAX_CELSIUS) temp_clamped =  TEMP_MAX_CELSIUS;
        if (temp_clamped < TEMP_MIN_CELSIUS) temp_clamped = TEMP_MIN_CELSIUS;
        int16_t temp_raw = (int16_t)lroundf(temp_clamped * 100.0f);

        /* Write temperature into register */
        write_register(REG_IN_TEMP_RAW_1, &temp_raw, sizeof(temp_raw), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Read offset & gain */
        int16_t offset;
        int16_t gain;
        if (!read_register(REG_CAL_TEMP_OFFSET_1, &offset,   sizeof(offset))) return;
        if (!read_register(REG_CAL_TEMP_GAIN_1,   &gain,     sizeof(gain))) return;
        if (gain == 0) gain = 1000;  /* Default: 1.0 (no gain change) */

        /* temp_cal = (temp_raw * gain / 1000) + offset */
        int32_t temp_cal_32 = ((int32_t)temp_raw * gain) / 1000 + offset;

        /* Clamp to int16 range */
        if (temp_cal_32 >  32767) temp_cal_32 =  32767;
        if (temp_cal_32 < -32768) temp_cal_32 = -32768;

        int16_t temp_cal = (int16_t)temp_cal_32;
        write_register(REG_IN_TEMP_CAL_1, &temp_cal, sizeof(temp_cal), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Reset reading age */
        uint32_t zero = 0;
        write_register(REG_IN_READING_AGE_1, &zero, sizeof(zero), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Set data fresh flag */
        uint8_t fresh = 1;
        write_register(REG_IN_DATA_FRESH, &fresh, sizeof(fresh), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Update sensor status (Bit 0 = OK) */
        uint8_t status = 0x01;
        write_register(REG_IN_SENSOR_STATUS, &status, sizeof(status), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Increment read count */
        increment_register_u32(REG_IN_READ_COUNT_1);

        /* Set flag for update reading age */
        last_successful_read_uptime = millis64();
        has_successful_read = true;
    }
    else
    {
        /* Map DHT error to sensor status bitfield */
        if (data.status > DHT_ERROR_INTERNAL) data.status = DHT_ERROR_INTERNAL;
        uint8_t status = 1 << data.status;
        write_register(REG_IN_SENSOR_STATUS, &status, sizeof(status), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);

        /* Increment fail count */
        increment_register_u32(REG_IN_FAIL_COUNT_1);
    }

    return;
}

/**
 * @brief Edge interrupt handler for the DHT data pin.
 *
 * Records only a timestamp per call; no decoding or blocking work is
 * performed here so the serial protocol's own interrupts are not
 * delayed.
 */
static void dht_edge_isr()
{
    if (captureActive && edgeIdx < DHT_EDGE_COUNT)
        edgeTs[edgeIdx++] = micros();
}

/**
 * @brief Starts a new, non-blocking DHT read cycle.
 *
 * No-op if a read is already in progress.
 */
static void dht_start_read()
{
    if (dhtState != DHT_IDLE) return;

    pinMode(DHT22_PIN, OUTPUT);
    digitalWrite(DHT22_PIN, LOW);
    dhtStateTimer = millis();
    dhtState = DHT_START_LOW;
}

/**
 * @brief Decodes 40 data bits from the captured edge timestamps.
 *
 * @param idxCount  Number of edges captured for this frame
 * @param bytesOut  Destination buffer for the 5 decoded raw bytes
 * @return true if a full frame was captured and decoded, false if the
 *         frame is incomplete (e.g. due to a timeout)
 */
static bool dht_decode(uint8_t idxCount, uint8_t bytesOut[5])
{
    if (idxCount < DHT_EDGE_COUNT) return false;

    uint32_t ts[DHT_EDGE_COUNT];
    noInterrupts();
    memcpy((void *)ts, (const void *)edgeTs, sizeof(ts));
    interrupts();

    bytesOut[0] = bytesOut[1] = bytesOut[2] = bytesOut[3] = bytesOut[4] = 0;

    /* High-pulse threshold to distinguish bit 0/1, depends on detected sensor type */
    uint16_t highThresholdUs = (temp_sensor.getType() == DHT22) ? 40 : 50;

    for (uint8_t i = 0; i < 40; i++)
    {
        uint32_t highStart = ts[3 + 2 * i];
        uint32_t bitEnd     = ts[4 + 2 * i];
        uint32_t highDuration = bitEnd - highStart;

        uint8_t bit = (highDuration > highThresholdUs) ? 1 : 0;
        bytesOut[i / 8] = (bytesOut[i / 8] << 1) | bit;
    }
    return true;
}

/**
 * @brief Validates and forwards a captured DHT frame to the sensor library.
 *
 * On successful checksum verification, injects the raw bytes into
 * MyDHT via setRawBytes()/read() (testMode) so calibration and
 * derived values keep using the library's existing logic, then
 * forwards the result to temp_sensor_callback() unchanged.
 * On failure, updates the sensor status/fail-count registers directly.
 *
 * @param idxCount  Number of edges captured for this frame
 */
static void dht_handle_frame(uint8_t idxCount)
{
    uint8_t b[5];

    if (!dht_decode(idxCount, b))
    {
        uint8_t status = 1 << DHT_ERROR_BIT_TIMEOUT;
        write_register(REG_IN_SENSOR_STATUS, &status, sizeof(status), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
        increment_register_u32(REG_IN_FAIL_COUNT_1);
        return;
    }

    uint8_t sum = b[0] + b[1] + b[2] + b[3];
    if (sum != b[4])
    {
        uint8_t status = 1 << DHT_ERROR_CHECKSUM;
        write_register(REG_IN_SENSOR_STATUS, &status, sizeof(status), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
        increment_register_u32(REG_IN_FAIL_COUNT_1);
        return;
    }

    temp_sensor.setRawBytes(b[0], b[1], b[2], b[3], b[4]);
    DHTError err = temp_sensor.read(); /* testMode: sanityCheck() only, no hardware access */

    DHTData data = temp_sensor.getData();
    data.status = err;

    temp_sensor_callback(data);
}

/**
 * @brief Returns the required LOW hold time for the start signal, depending on sensor type.
 * @return Hold time in ms (DHT11: 18ms, DHT22: 2ms; conservative 18ms fallback if type is not yet known)
 */
static uint16_t dht_start_low_ms()
{
    switch (temp_sensor.getType())
    {
    case DHT22: return 2;
    case DHT11: return 18;
    default:    return 18; // DHT_AUTO / unbekannt: sicherer, längerer Wert
    }
}

/**
 * @brief Advances the DHT capture state machine.
 *
 * Must be called repeatedly (e.g. from update_temp_sensor()). Handles:
 *   - DHT_START_LOW:  holds the start pulse, then arms edge capture
 *   - DHT_WAIT_FRAME: waits for a full frame or a timeout, then hands
 *                      the result to dht_handle_frame()
 */
static void dht_process()
{
    switch (dhtState)
    {
    case DHT_IDLE:
        break;

    case DHT_START_LOW:
        if (millis() - dhtStateTimer >= dht_start_low_ms()) 
        {
            noInterrupts();
            edgeIdx = 0;
            captureActive = true;
            interrupts();

            digitalWrite(DHT22_PIN, HIGH);
            delayMicroseconds(30);
            pinMode(DHT22_PIN, INPUT_PULLUP);

            attachInterrupt(digitalPinToInterrupt(DHT22_PIN), dht_edge_isr, CHANGE);

            dhtStateTimer = micros();
            dhtState = DHT_WAIT_FRAME;
        }
        break;

    case DHT_WAIT_FRAME:
    {
        const uint32_t FRAME_TIMEOUT_US = 8000; /* Start+ACK+40 bits ~4-5ms, plus margin */

        uint8_t idxSnapshot;
        noInterrupts();
        idxSnapshot = edgeIdx;
        interrupts();

        if (idxSnapshot >= DHT_EDGE_COUNT || (micros() - dhtStateTimer > FRAME_TIMEOUT_US))
        {
            detachInterrupt(digitalPinToInterrupt(DHT22_PIN));
            captureActive = false;

            dht_handle_frame(idxSnapshot);
            dhtState = DHT_IDLE;
        }
        break;
    }
    }
}

/* Function definition */
void init_temp_sensor(void)
{
    temp_sensor.testMode = true; /* Required for setRawBytes()/read() to accept injected bytes */
    return;
}

void update_temp_sensor(void)
{
    /* Check if signal led should be turn off */
    if (digitalRead(LED_BUILTIN))
    {
        if ((millis64() - led_timestamp) >= 100) digitalWrite(LED_BUILTIN, LOW);
    }

    /* Update reading age */
    if (has_successful_read)
    {
        uint32_t age = (uint32_t)(millis64() - last_successful_read_uptime);
        write_register(REG_IN_READING_AGE_1, &age, sizeof(age), REGISTER_FORCE_WRITE, REGISTER_NO_CALLBACK);
    }

    /* Start a new read once the minimum interval has elapsed */
    if (dhtState == DHT_IDLE && millis64() > next_callback_time)
    {
        next_callback_time = millis64() + temp_sensor.getMinReadInterval();
        dht_start_read();
    }

    dht_process();
    return;
}