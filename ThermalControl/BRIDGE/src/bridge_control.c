/* Header */
#include "bridge_control.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Static local variables */
static bridge_control_config_t current_config = { .mode = BRIDGE_CONTROL_MODE_NONE };
static CRITICAL_SECTION config_lock;

/* Function definitions */
void bridge_control_init(void)
{
	InitializeCriticalSection(&config_lock);
	return;
}

void bridge_publish_control_config(const bridge_control_config_t* config)
{
	if (config == NULL) return;
	EnterCriticalSection(&config_lock);
	current_config = *config;
	LeaveCriticalSection(&config_lock);
	return;
}

bool bridge_get_control_config(bridge_control_config_t* out)
{
	if (out == NULL) return false;
	EnterCriticalSection(&config_lock);
	*out = current_config;
	LeaveCriticalSection(&config_lock);
	return true;
}
