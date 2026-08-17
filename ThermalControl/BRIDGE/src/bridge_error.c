/* Header */
#include "bridge_error.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

/* Defines */
#define ERROR_QUEUE_CAPACITY		16

/* Static local variables */
static error_entry_t error_queue[ERROR_QUEUE_CAPACITY];
static size_t error_write_index = 0;
static size_t error_read_index = 0;
static size_t error_count = 0;
static CRITICAL_SECTION error_queue_lock;

/* Function definitions */
void bridge_error_init(void)
{
	InitializeCriticalSection(&error_queue_lock);
	error_write_index = 0;
	error_read_index = 0;
	error_count = 0;
	return;
}

void bridge_publish_error(const error_code_t code, const char* message)
{
	if (code == ERROR_NONE) return;
	if (message == NULL) return;

	EnterCriticalSection(&error_queue_lock);

	if (error_count < ERROR_QUEUE_CAPACITY)
	{
		error_entry_t* entry = &error_queue[error_write_index];
		entry->code = code;
		strncpy(entry->message, message, ERROR_MESSAGE_MAX_LENGTH - 1);
		entry->message[ERROR_MESSAGE_MAX_LENGTH - 1] = '\0';
		error_write_index = (error_write_index + 1) % ERROR_QUEUE_CAPACITY;
		error_count++;
	}

	LeaveCriticalSection(&error_queue_lock);

	return;
}

bool bridge_pull_error(error_entry_t* out)
{
	if (out == NULL) return false;

	bool error_found = false;
	EnterCriticalSection(&error_queue_lock);
	if (error_count > 0)
	{
		*out = error_queue[error_read_index];
		error_read_index = (error_read_index + 1) % ERROR_QUEUE_CAPACITY;
		error_count--;
		error_found = true;
	}
	LeaveCriticalSection(&error_queue_lock);

	return error_found;
}