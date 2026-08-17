#ifndef BUTTON_H
#define BUTTON_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "lang/lang.h"

/* Type definitions */
typedef void (*button_callback_t)(Clay_ElementId element_id, Clay_PointerData data, void* user_data);

typedef struct
{
	string_id_t enabled_label;
	string_id_t disabled_label;
	string_id_t toggle_on_label;
	string_id_t toggle_off_label;
} button_labels_t;

typedef struct
{
	const Clay_Color* base_color;
	const Clay_Color* enabled_color;
	const Clay_Color* disabled_color;
	const Clay_Color* toggle_on_color;
	const Clay_Color* toggle_off_color;
} button_color_override_t;

typedef struct
{
	uint32_t uid;
	button_labels_t labels;
	bool enabled;
	bool is_toggle_button;
	bool toggled_state;
	button_color_override_t color_override;
	button_callback_t callback;
	void* user_data;
} button_config_t;

/* Function declaration */
void button_init(button_config_t* config);
void button_render(button_config_t* config);

#endif // BUTTON_H