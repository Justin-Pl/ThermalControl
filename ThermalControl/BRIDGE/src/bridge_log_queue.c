/* Header */
#include "bridge_log_queue.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Defines */
#define LOG_QUEUE_CAPACITY			128

/* Type definitions */
typedef struct
{
    console_message_t buffer[LOG_QUEUE_CAPACITY];
    volatile LONG write_index;
    volatile LONG read_index;
} log_queue_t;

/* Static local variables */
static log_queue_t queue;

/* Static function definitions */
static uint64_t get_current_utc_ms(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli = { 0 };
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    uint64_t ms_since_1601 = uli.QuadPart / 10000ULL;
    const uint64_t EPOCH_DIFF_MS = 11644473600000ULL;
    return ms_since_1601 - EPOCH_DIFF_MS;
}

/* Function definitions */
void bridge_log_queue_init(void)
{
    queue.write_index = 0;
    queue.read_index = 0;
    return;
}

bool bridge_log_queue_push(const char* text)
{
    if (text == NULL) return false;

    LONG write_idx = queue.write_index;
    LONG next_write_idx = (write_idx + 1) % LOG_QUEUE_CAPACITY;
    if (next_write_idx == queue.read_index) return false;  

    console_message_t* msg = &queue.buffer[write_idx];
    strncpy(msg->plain_text, text, sizeof(msg->plain_text) - 1);
    msg->plain_text[sizeof(msg->plain_text) - 1] = '\0';
    msg->timestamp_ms = get_current_utc_ms();

    MemoryBarrier();
    queue.write_index = next_write_idx;
    return true;
}

bool bridge_log_queue_pull(console_message_t* out)
{
    if (out == NULL) return false;

    LONG read_idx = queue.read_index;
    if (read_idx == queue.write_index) return false;

    *out = queue.buffer[read_idx];
    MemoryBarrier();
    queue.read_index = (read_idx + 1) % LOG_QUEUE_CAPACITY;
    return true;
}