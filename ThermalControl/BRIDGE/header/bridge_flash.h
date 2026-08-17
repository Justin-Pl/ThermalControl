#ifndef BRIDGE_FLASH_H
#define BRIDGE_FLASH_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "firmware_files.h"

/* Typedefinitions */
typedef enum
{
	BRIDGE_FLASH_STATE_IDLE = 0,
	BRIDGE_FLASH_READ_FILE,
	BRIDGE_FLASH_CHECKING_SIGNATURE,
	BRIDGE_FLASH_WRITING_FIRMWARE,
	BRIDGE_FLASH_READING_FIRMWARE,
	BRIDGE_FLASH_VERIFYING_FIRMWARE,
	BRIDGE_FLASH_COMPLETED,
	BRIDGE_FLASH_ERROR
} bridge_flash_state_t;

typedef struct
{
	char firmware_file[FIRMWARE_FILE_NAME_MAX];
} bridge_flash_params_t;

typedef struct
{
	uint8_t progress_percentage;
	bridge_flash_state_t state;
} bridge_flash_progress_t;

/* Function declarations */
void bridge_flash_init(void);
void bridge_publish_flash_request(const bridge_flash_params_t* params);
bool bridge_get_flash_request_if_changed(bridge_flash_params_t* out);
void bridge_publish_flash_progress(const bridge_flash_progress_t* progress);
bool bridge_get_flash_progress(bridge_flash_progress_t* out);

#endif // BRIDGE_FLASH_H