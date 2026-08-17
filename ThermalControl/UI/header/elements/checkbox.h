#ifndef CHECKBOX_H
#define CHECKBOX_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include "ui_constants.h"
#include "lang/font_manager.h"
#include "renderer/clay.h"

/* Type definitions */
typedef void (*checkbox_callback_t)(Clay_ElementId element_id, Clay_PointerData data, void* user_data);

typedef struct
{
	uint32_t uid;
	bool enabled;
	bool checked;
	checkbox_callback_t callback;
	void* user_data;
} checkbox_config_t;

/* Function declarations */
void checkbox_init(checkbox_config_t* config);
void checkbox_render(checkbox_config_t* config);

#endif // CHECKBOX_H