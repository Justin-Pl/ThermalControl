/* Header */
#include "elements/button.h"

/* Defines */
#define BUTTON_INVALID_UID		0

/* Static local variables */
static uint32_t uid_counter = 1;

/* Static function definition */
static void button_handle_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    button_config_t* config = (button_config_t*)user_data;

    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

    if (config->is_toggle_button)
    {
        config->toggled_state = !config->toggled_state;
    }

    if (config->callback != NULL)
    {
        config->callback(element_id, data, config->user_data);
    }

    return;
}

/* Function definition */
void button_init(button_config_t* config)
{
	if (config == NULL) return;

	config->uid = uid_counter++;

	return;
}

void button_render(button_config_t* config)
{
	if (config == NULL) return;
	if (config->uid <= BUTTON_INVALID_UID) return;

	string_id_t label_id;
	Clay_Color background_color;
	Clay_Color text_color;
    Clay_ElementId button_id = CLAY_SIDI(CLAY_STRING("Button"), config->uid);
    bool is_hovered = Clay_PointerOver(button_id);

    if (!config->enabled)
    {
        label_id = config->labels.disabled_label;
        background_color = config->color_override.base_color != NULL
            ? *config->color_override.base_color
            : get_color(APP_COLOR_BUTTON_BACKGROUND_DISABLED);
        text_color = config->color_override.disabled_color != NULL
            ? *config->color_override.disabled_color
            : get_color(APP_COLOR_BUTTON_TEXT_DISABLED);
    }
    else if (config->is_toggle_button)
    {
        label_id = config->toggled_state ? config->labels.toggle_on_label : config->labels.toggle_off_label;
        const Clay_Color* toggle_color = config->toggled_state
            ? config->color_override.toggle_on_color
            : config->color_override.toggle_off_color;
        background_color = toggle_color != NULL
            ? *toggle_color
            : (config->toggled_state ? get_color(APP_COLOR_TOGGLE_ON) : get_color(APP_COLOR_TOGGLE_OFF));
        text_color = config->color_override.enabled_color != NULL
            ? *config->color_override.enabled_color
            : get_color(APP_COLOR_TEXT);
    }
    else
    {
        label_id = config->labels.enabled_label;
        background_color = is_hovered
            ? get_color(APP_COLOR_HOVER)
            : (config->color_override.base_color ? *config->color_override.base_color : get_color(APP_COLOR_ITEM_BACKGROUND));
        text_color = config->color_override.enabled_color != NULL
            ? *config->color_override.enabled_color
            : get_color(APP_COLOR_TEXT);
    }

    CLAY(CLAY_IDI("Button", config->uid),
    {
        .layout = 
        {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(HALF_PADDING), 
        },

        .backgroundColor = background_color,
        .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
    })
    {
        if (config->enabled)
        {
            Clay_OnHover(button_handle_interaction, config);
        }

        CLAY_TEXT(get_label(label_id),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = text_color
        });
    }
    
	return;
}