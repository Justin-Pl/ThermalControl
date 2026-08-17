/* Header */
#include "elements/checkbox.h"

/* Defines */
#define CHECKBOX_INVALID_UID		0
#define CHECKBOX_SIZE			    16
#define CHECKBOX_BORDER_WIDTH		2

/* Static local variables */
static uint32_t uid_counter = 1;

/* Static function definition */
static void checkbox_handle_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    checkbox_config_t* config = (checkbox_config_t*)user_data;

    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

	config->checked = !config->checked;

    if (config->callback != NULL)
    {
        config->callback(element_id, data, config->user_data);
    }

    return;
}

/* Function definition */
void checkbox_init(checkbox_config_t* config)
{
    if (config == NULL) return;

    config->uid = uid_counter++;

    return;
}

void checkbox_render(checkbox_config_t* config)
{
    if (config == NULL) return;
    if (config->uid <= CHECKBOX_INVALID_UID) return;

    Clay_ElementId checkbox_id = CLAY_SIDI(CLAY_STRING("Checkbox"), config->uid);
    bool is_hovered = Clay_PointerOver(checkbox_id);

    Clay_Color background_color;
    Clay_Color border_color = config->checked ? get_color(APP_COLOR_TOGGLE_ON) : get_color(APP_COLOR_PANEL_OUTLINE);
    uint16_t border_width = config->checked ? 0 : CHECKBOX_BORDER_WIDTH;
    if (config->enabled)
    {
        if (is_hovered && !config->checked)
        {
            background_color = get_color(APP_COLOR_HOVER);
        }
        else if (config->checked)
		{
			background_color = get_color(APP_COLOR_TOGGLE_ON);
		}
    }
    else
    {
        background_color = (Clay_Color){ 0, 0, 0, 0 };
    }

    CLAY(CLAY_IDI("Checkbox", config->uid),
    {
        .layout =
        {
            .sizing = { .width = CLAY_SIZING_FIXED(CHECKBOX_SIZE), .height = CLAY_SIZING_FIXED(CHECKBOX_SIZE) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
        },
        .cornerRadius = CLAY_CORNER_RADIUS(MIN_RADIUS),
        .backgroundColor = background_color,
        .border =
        {
            .color = border_color,
            .width = { border_width, border_width, border_width, border_width }
        }
    })
    {
        if (config->enabled)
        {
            Clay_OnHover(checkbox_handle_interaction, config);
        }

        if (config->checked)
        {
            CLAY_TEXT(CLAY_STRING("x"),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });
        }
    }

    return;
}