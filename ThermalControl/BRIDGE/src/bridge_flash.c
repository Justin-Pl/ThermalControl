/* Header */
#include "bridge_flash.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Static local variables */
static bridge_flash_params_t current_request;
static bool has_pending_request = false;
static CRITICAL_SECTION request_lock;
static bridge_flash_progress_t current_progress = { .progress_percentage = 0, .state = BRIDGE_FLASH_STATE_IDLE };
static CRITICAL_SECTION progress_lock;

/* Function definitions */
void bridge_flash_init(void)
{
	InitializeCriticalSection(&request_lock);
	InitializeCriticalSection(&progress_lock);
	return;
}

void bridge_publish_flash_request(const bridge_flash_params_t* params)
{
	if (params == NULL) return;
	if (!strlen(params->firmware_file)) return;
	EnterCriticalSection(&request_lock);
	current_request = *params;
	has_pending_request = true;
	LeaveCriticalSection(&request_lock);
	return;
}

bool bridge_get_flash_request_if_changed(bridge_flash_params_t* out)
{
	if (out == NULL) return false;
	bool got_one = false;
	EnterCriticalSection(&request_lock);
	if (has_pending_request)
	{
		*out = current_request;
		has_pending_request = false;
		got_one = true;
	}
	LeaveCriticalSection(&request_lock);
	return got_one;
}

void bridge_publish_flash_progress(const bridge_flash_progress_t* progress)
{
	if (progress == NULL) return;
	EnterCriticalSection(&progress_lock);
	current_progress = *progress;
	LeaveCriticalSection(&progress_lock);
	return;
}

bool bridge_get_flash_progress(bridge_flash_progress_t* out)
{
	if (out == NULL) return false;
	EnterCriticalSection(&progress_lock);
	*out = current_progress;
	LeaveCriticalSection(&progress_lock);
	return true;
}