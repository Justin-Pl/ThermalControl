#ifndef BRIDGE_ERROR_H
#define	BRIDGE_ERROR_H

/* Libraries */
#include <stdbool.h>

/* Defines */
#define ERROR_MESSAGE_MAX_LENGTH 128

/* Type definitions */
typedef enum
{
	ERROR_NONE = 0,

	ERROR_GENERAL = 1,
	ERROR_SAVE_DIALOG = 2,
	ERROR_EXPORT_FAILED = 3,
} error_code_t;

typedef struct
{
	error_code_t code;
	char message[ERROR_MESSAGE_MAX_LENGTH];
} error_entry_t;

/* Function declarations */
void bridge_error_init(void);
void bridge_publish_error(const error_code_t code, const char* message);
bool bridge_pull_error(error_entry_t* out);

#endif // BRIDGE_ERROR_H