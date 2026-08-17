/* Header */
#include "ui_constants.h"

/* Static local variables */
static const Clay_Color colors[COLOR_PALETTE_COUNT][APP_COLOR_COUNT] =
{
    [COLOR_PALETTE_DARK] =
    {
        [APP_COLOR_TEXT] = ((Clay_Color) { .r = 228, .g = 230, .b = 234, .a = 255 }),
        [APP_COLOR_TEXT_WARNING] = ((Clay_Color) { .r = 224, .g = 74, .b = 74, .a = 255 }),
        [APP_COLOR_BACKGROUND] = ((Clay_Color) { .r = 26,  .g = 29,  .b = 33,  .a = 255 }),
        [APP_COLOR_BOX_BACKGROUND] = ((Clay_Color) { .r = 34,  .g = 38,  .b = 43,  .a = 255 }),
        [APP_COLOR_PANEL_BACKGROUND] = ((Clay_Color) { .r = 60,  .g = 67,  .b = 77,  .a = 255 }),
        [APP_COLOR_PANEL_OUTLINE] = ((Clay_Color) { .r = 92,  .g = 99,  .b = 110, .a = 255 }),
        [APP_COLOR_ITEM_BACKGROUND] = ((Clay_Color) { .r = 40,  .g = 45,  .b = 52,  .a = 255 }),
        [APP_COLOR_ITEM_HOVER_BACKGROUND] = ((Clay_Color) { .r = 74,  .g = 81,  .b = 92,  .a = 255 }),
        [APP_COLOR_HOVER] = ((Clay_Color) { .r = 82,  .g = 89,  .b = 100, .a = 255 }),
        [APP_COLOR_GRAPH_GRID] = ((Clay_Color) { .r = 52,  .g = 56,  .b = 63,  .a = 255 }),
        [APP_COLOR_GRAPH_TEMP] = ((Clay_Color) { .r = 201, .g = 138, .b = 75,  .a = 255 }),
        [APP_COLOR_GRAPH_PWM] = ((Clay_Color) { .r = 91,  .g = 154, .b = 173, .a = 255 }),
        [APP_COLOR_BUTTON_TEXT_DISABLED] = ((Clay_Color) { .r = 107, .g = 111, .b = 118, .a = 255 }),
        [APP_COLOR_BUTTON_BACKGROUND_DISABLED] = ((Clay_Color) { .r = 44,  .g = 48,  .b = 54,  .a = 255 }),
        [APP_COLOR_TOGGLE_ON] = ((Clay_Color) { .r = 76,  .g = 140, .b = 107, .a = 255 }),
        [APP_COLOR_TOGGLE_OFF] = ((Clay_Color) { .r = 58,  .g = 63,  .b = 71,  .a = 255 }),
        [APP_COLOR_PROGRESS_TRACK] = ((Clay_Color) { .r = 38, .g = 139, .b = 210, .a = 255 }),
        [APP_COLOR_INPUT_FOCUS] = ((Clay_Color) { .r = 38, .g = 139, .b = 210, .a = 255 })
    },
    [COLOR_PALETTE_LIGHT] =
    {
        [APP_COLOR_TEXT] = ((Clay_Color) { .r = 43,  .g = 46,  .b = 51,  .a = 255 }),
        [APP_COLOR_TEXT_WARNING] = ((Clay_Color) { .r = 178, .g = 34, .b = 34, .a = 255 }),
        [APP_COLOR_BACKGROUND] = ((Clay_Color) { .r = 241, .g = 242, .b = 244, .a = 255 }),
        [APP_COLOR_BOX_BACKGROUND] = ((Clay_Color) { .r = 255, .g = 255, .b = 255, .a = 255 }),
        [APP_COLOR_PANEL_BACKGROUND] = ((Clay_Color) { .r = 218, .g = 222, .b = 227, .a = 255 }),
        [APP_COLOR_PANEL_OUTLINE] = ((Clay_Color) { .r = 190, .g = 195, .b = 201, .a = 255 }),
        [APP_COLOR_ITEM_BACKGROUND] = ((Clay_Color) { .r = 234, .g = 235, .b = 237, .a = 255 }),
        [APP_COLOR_ITEM_HOVER_BACKGROUND] = ((Clay_Color) { .r = 206, .g = 210, .b = 216, .a = 255 }),
        [APP_COLOR_HOVER] = ((Clay_Color) { .r = 212, .g = 216, .b = 221, .a = 255 }), 
        [APP_COLOR_GRAPH_GRID] = ((Clay_Color) { .r = 227, .g = 229, .b = 232, .a = 255 }),
        [APP_COLOR_GRAPH_TEMP] = ((Clay_Color) { .r = 203, .g = 75,  .b = 22,  .a = 255 }),
        [APP_COLOR_GRAPH_PWM] = ((Clay_Color) { .r = 38,  .g = 139, .b = 210, .a = 255 }),
        [APP_COLOR_BUTTON_TEXT_DISABLED] = ((Clay_Color) { .r = 168, .g = 172, .b = 178, .a = 255 }),
        [APP_COLOR_BUTTON_BACKGROUND_DISABLED] = ((Clay_Color) { .r = 237, .g = 238, .b = 240, .a = 255 }), 
        [APP_COLOR_TOGGLE_ON] = ((Clay_Color) { .r = 63,  .g = 122, .b = 92,  .a = 255 }),
        [APP_COLOR_TOGGLE_OFF] = ((Clay_Color) { .r = 211, .g = 214, .b = 218, .a = 255 }),
        [APP_COLOR_PROGRESS_TRACK] = ((Clay_Color) { .r = 38, .g = 139, .b = 210, .a = 255 }),
        [APP_COLOR_INPUT_FOCUS] = ((Clay_Color) { .r = 38, .g = 139, .b = 210, .a = 255 })
    }
};

static color_palette_t current_palette = COLOR_PALETTE_DARK;
static const Clay_Color fallback = { .r = 0, .g = 0, .b = 0, .a = 255 };

/* Function definition */
void set_color_palette(const color_palette_t palette)
{
	if ((palette < 0) || (palette >= COLOR_PALETTE_COUNT)) return;

	current_palette = palette;

	return;
}

color_palette_t get_current_color_palette(void)
{
    return current_palette;
}

const Clay_Color get_color(const app_color_t color_code)
{
	if ((color_code < 0) || (color_code >= APP_COLOR_COUNT)) return fallback;

	return colors[current_palette][color_code];
}

const Color get_color_raylib(const app_color_t color_code)
{
	Color color = { .r = fallback.r, .g = fallback.g, .b = fallback.b, .a = fallback.a };
	if ((color_code < 0) || (color_code >= APP_COLOR_COUNT)) return color;

	color.r = colors[current_palette][color_code].r;
	color.g = colors[current_palette][color_code].g;
	color.b = colors[current_palette][color_code].b;
	color.a = colors[current_palette][color_code].a;
	return color;
}