#ifndef BRIDGE_LOG_QUEUE_H
#define BRIDGE_LOG_QUEUE_H

/* Libraries */
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* Defines */
#define CONSOLE_MAX_MESSAGE_LENGTH  256

/* Type definitions */
typedef struct
{
	char plain_text[CONSOLE_MAX_MESSAGE_LENGTH];
	uint64_t timestamp_ms;
} console_message_t;

/* Function definitions */
void bridge_log_queue_init(void);
bool bridge_log_queue_push(const char* text);        
bool bridge_log_queue_pull(console_message_t* out);

#endif // BRIDGE_LOG_QUEUE_H