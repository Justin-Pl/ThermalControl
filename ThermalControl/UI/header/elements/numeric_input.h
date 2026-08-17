#ifndef NUMERIC_INPUT_H
#define NUMERIC_INPUT_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "renderer/raylib.h"

/* Defines */
#define NUMERIC_INPUT_TEXT_BUFFER_SIZE			16

/* Type definitions */
typedef void (*numeric_input_callback_t)(float value, void* user_data);

typedef struct
{
	uint32_t uid;
	float value;
	float min;
	float max;
	bool allow_decimal;
	uint8_t decimal_places;
	bool focused;
	char text_buffer[NUMERIC_INPUT_TEXT_BUFFER_SIZE];
	float calculated_width;
	uint64_t last_keypress_frame_time;
	numeric_input_callback_t on_commit;
	void* user_data;
} numeric_input_config_t;

/* Function declarations */
void numeric_input_set_font(Font font);
void numeric_input_init(numeric_input_config_t* config);
void numeric_input_update(numeric_input_config_t* config);
void numeric_input_render(numeric_input_config_t* config);

#endif // NUMERIC_INPUT_H
