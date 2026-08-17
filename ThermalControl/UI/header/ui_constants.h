#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

/* Libraries */
#include "renderer/clay.h"
#include "lang/font_manager.h"

/* Defines */
#define BASE_FONT_FAMILY		(font_family_id_t)FAMILY_OPEN_SANS
#define BASE_FONT_SIZE			(font_size_t)FONT_SIZE_18
#define STANDARD_PADDING		16
#define HALF_PADDING			8
#define MIN_PADDING				4
#define STANDARD_RADIUS			8
#define MIN_RADIUS				4

/* General constants */
#define SCREEN_WIDTH		1024
#define SCREEN_HEIGHT		768
#define WINDOW_TITLE		"ThermalControl"
#define CONSOLE_BAR_HEIGHT	50
#define CONSOLE_HEIGHT		200

/* Type definitions */
typedef enum
{
	APP_COLOR_TEXT,
	APP_COLOR_TEXT_WARNING,
	APP_COLOR_BACKGROUND,
	APP_COLOR_BOX_BACKGROUND,
	APP_COLOR_PANEL_BACKGROUND,
	APP_COLOR_PANEL_OUTLINE,
	APP_COLOR_ITEM_BACKGROUND,
	APP_COLOR_ITEM_HOVER_BACKGROUND,
	APP_COLOR_HOVER,
	APP_COLOR_GRAPH_GRID,       
	APP_COLOR_GRAPH_TEMP,        
	APP_COLOR_GRAPH_PWM,
	APP_COLOR_BUTTON_TEXT_DISABLED,   
	APP_COLOR_BUTTON_BACKGROUND_DISABLED,
	APP_COLOR_TOGGLE_ON,                  
	APP_COLOR_TOGGLE_OFF,
	APP_COLOR_PROGRESS_TRACK,
	APP_COLOR_INPUT_FOCUS,

	APP_COLOR_COUNT
} app_color_t;

typedef enum
{
	COLOR_PALETTE_DARK,
	COLOR_PALETTE_LIGHT,

	COLOR_PALETTE_COUNT
} color_palette_t;

/* Function declaration */
void set_color_palette(const color_palette_t palette);
color_palette_t get_current_color_palette(void);
const Clay_Color get_color(const app_color_t color_code);
const Color get_color_raylib(const app_color_t color_code);

#endif // UI_CONSTANTS_H