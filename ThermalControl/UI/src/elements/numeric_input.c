/* Header */
#include "elements/numeric_input.h"

/* Defines */
#define NUMERIC_INPUT_BORDER_WIDTH          2
#define NUMERIC_INPUT_MIN_WIDTH             32
#define NUMERIC_INPUT_HEIGHT                24
#define NUMERIC_INPUT_CURSOR_WIDTH          2
#define NUMERIC_INPUT_CURSOR_GAP            2
#define NUMERIC_INPUT_FONT_SPACING          1
#define NUMERIC_INPUT_BLINK_PERIOD_S        1.0f

/* Static local variables */
static uint32_t uid_counter = 1;
static uint32_t focused_uid = 0;
static numeric_input_config_t* last_config = NULL;
static Font numeric_input_font;

/* Static function definitions */
static uint8_t numeric_input_calculate_max_int_digits(float min_value, float max_value)
{
    float max_abs = fabsf(min_value);
    if (fabsf(max_value) > max_abs) max_abs = fabsf(max_value);

    int int_part = (int)max_abs;
    uint8_t digit_count = 1;
    while (int_part >= 10)
    {
        int_part /= 10;
        digit_count++;
    }
    return digit_count;
}

static bool numeric_input_build_widest_string(const numeric_input_config_t* config, char* out, size_t out_size)
{
    /* Check arguments */
    if (config == NULL) return false;
    if ((out == NULL) || !out_size) return false;

    uint8_t int_digits = numeric_input_calculate_max_int_digits(config->min, config->max);

    /* Check if range is under 1 */
    bool range_under_one = config->allow_decimal && (fabsf(config->min) < 1.0f) && (fabsf(config->max) < 1.0f);
    if (range_under_one) int_digits = 1;

    /* Check size */
    size_t widest_string_size = int_digits;
    if (widest_string_size >= out_size) return false;

    /* Check for negativ sign */
    size_t pos = 0;
    if (config->min < 0.0f)
    {
        widest_string_size++;
        if (widest_string_size >= out_size) return false;
        out[pos++] = '-';
    }

    /* Fill string with digits */
    for (size_t digit_index = 0; digit_index < int_digits; digit_index++) out[pos++] = '9';

    /* Check for decimal & decimal places */
    if (config->allow_decimal && (config->decimal_places > 0))
    {
        widest_string_size += config->decimal_places + 1;
        if (widest_string_size >= out_size) return false;
        out[pos++] = '.';
        for (size_t decimal_index = 0; decimal_index < config->decimal_places; decimal_index++) out[pos++] = '9';
    }
    out[pos] = '\0';
    return true;
}

static float numeric_input_calculate_width(const numeric_input_config_t* config)
{
    char widest[32];
    if (!numeric_input_build_widest_string(config, widest, sizeof(widest))) return -1.0f;

    Vector2 text_size = MeasureTextEx(numeric_input_font, widest, (float)get_font_size_int(BASE_FONT_SIZE), NUMERIC_INPUT_FONT_SPACING);

    return text_size.x + NUMERIC_INPUT_CURSOR_GAP + NUMERIC_INPUT_CURSOR_WIDTH + (2.0f * HALF_PADDING);
}

static void numeric_input_format_display(numeric_input_config_t* config)
{
    if (config == NULL) return;

    if (config->allow_decimal)
    {
        char fmt[32];
        snprintf(fmt, sizeof(fmt), "%%.%df", config->decimal_places);
        snprintf(config->text_buffer, sizeof(config->text_buffer), fmt, config->value);
    }
    else
    {
        snprintf(config->text_buffer, sizeof(config->text_buffer), "%d", (int)config->value);
    }
    return;
}

static void numeric_input_commit(numeric_input_config_t* config)
{
    if (config == NULL) return;

    /* Copy text from numeric input */
    char normalized[NUMERIC_INPUT_TEXT_BUFFER_SIZE];
    strncpy(normalized, config->text_buffer, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';

    /* Replace ',' with '.' */
    for (char* current_char = normalized; *current_char != '\0'; current_char++)
    {
        if (*current_char == ',') *current_char = '.';
    }

    /* Parse to float */
    float parsed = (float)atof(normalized);

    /* Clamp to min, max */
    if (parsed < config->min) parsed = config->min;
    if (parsed > config->max) parsed = config->max;

    /* Copy parsed value & display */
    config->value = parsed;
    numeric_input_format_display(config);

    /* Call commit if configured */
    if (config->on_commit != NULL) config->on_commit(config->value, config->user_data);

    return;
}

static bool numeric_input_has_separator(const char* text)
{
    if (text == NULL) return false;
    return (strchr(text, '.') != NULL) || (strchr(text, ',') != NULL);
}

static bool numeric_input_leading_zero_blocks_append(const char* text, bool has_separator_already)
{
    if (text == NULL) return false;
    if (has_separator_already) return false;

    const char* current_char = text;
    if (*current_char == '-') current_char++;
    return (current_char[0] == '0') && (current_char[1] == '\0');
}

static bool numeric_input_candidate_in_range(const numeric_input_config_t* config, const char* candidate)
{
    /* Check arguments */
    if (config == NULL) return false;
    if (candidate == NULL) return false;

    /* If candidate is no number skip the check */
    if (candidate[0] == '\0') return true;
    if (strcmp(candidate, "-") == 0) return true;

    /* Normalize string for conversion */
    char normalized[32];
    strncpy(normalized, candidate, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    for (char* current_char = normalized; *current_char != '\0'; current_char++)
    {
        if (*current_char == ',') *current_char = '.';
    }

    /* Compare parsed value with min/max */
    float parsed = (float)atof(normalized);
    return (parsed >= config->min) && (parsed <= config->max);
}

static void numeric_input_try_append_digit(numeric_input_config_t* config, char digit)
{
    if (config == NULL) return;

    /* Copy text buffer */
    char candidate[sizeof(config->text_buffer)];
    strncpy(candidate, config->text_buffer, sizeof(candidate) - 1);
    candidate[sizeof(candidate) - 1] = '\0';

    /* Build candidate & append digit */
    size_t len = strlen(candidate);
    bool has_sep = numeric_input_has_separator(candidate);

    if (numeric_input_leading_zero_blocks_append(candidate, has_sep))
    {
        size_t zero_pos = (candidate[0] == '-') ? 1 : 0;
        candidate[zero_pos] = digit;
    }
    else if (has_sep && config->allow_decimal)
    {
        const char* sep = strpbrk(candidate, ".,");
        size_t decimals_so_far = strlen(sep + 1);
        if (((int)decimals_so_far >= config->decimal_places) || (len >= sizeof(candidate) - 1)) return;
        candidate[len] = digit;
        candidate[len + 1] = '\0';
    }
    else
    {
        if (len >= sizeof(candidate) - 1) return;
        candidate[len] = digit;
        candidate[len + 1] = '\0';
    }

    /* Check if candidate is in range */
    if (!numeric_input_candidate_in_range(config, candidate)) return;

    /* Copy candidate back if in range */
    strncpy(config->text_buffer, candidate, sizeof(config->text_buffer) - 1);
    config->text_buffer[sizeof(config->text_buffer) - 1] = '\0';
    return;
}

static void numeric_input_handle_keyboard(numeric_input_config_t* config)
{
    if (config == NULL) return;

    int current_char = GetCharPressed();
    while (current_char > 0)
    {
        size_t len = strlen(config->text_buffer);
        bool is_digit = (current_char >= '0') && (current_char <= '9');
        bool is_minus = (current_char == '-') && (len == 0) && (config->min < 0.0f);
        bool has_sep = numeric_input_has_separator(config->text_buffer);
        bool is_separator = config->allow_decimal && ((current_char == '.') || (current_char == ',')) && !has_sep && (len > 0);

        if (is_digit)
        {
            numeric_input_try_append_digit(config, (char)current_char); 
        }
        else if ((is_minus || is_separator) && (len < sizeof(config->text_buffer) - 1))
        {
            config->text_buffer[len] = (char)current_char;
            config->text_buffer[len + 1] = '\0';
        }

        config->last_keypress_frame_time = (uint64_t)(GetTime() * 1000.0);
        current_char = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        size_t len = strlen(config->text_buffer);
        if (len > 0) config->text_buffer[len - 1] = '\0';
        config->last_keypress_frame_time = (uint64_t)(GetTime() * 1000.0);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
    {
        numeric_input_commit(config);
        config->focused = false;
        focused_uid = 0;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        numeric_input_format_display(config);
        config->focused = false;
        focused_uid = 0;
    }
    return;
}

/* Function definitions */
void numeric_input_set_font(Font font)
{
    numeric_input_font = font;
    return;
}

void numeric_input_init(numeric_input_config_t* config)
{
    if (config == NULL) return;
    config->uid = uid_counter++;
    numeric_input_format_display(config);
    config->focused = false;

    float width = numeric_input_calculate_width(config);
    config->calculated_width = width < NUMERIC_INPUT_MIN_WIDTH ? NUMERIC_INPUT_MIN_WIDTH : width;
    return;
}

void numeric_input_update(numeric_input_config_t* config)
{
    if (config == NULL) return;

    numeric_input_format_display(config);

    return;
}

void numeric_input_render(numeric_input_config_t* config)
{
    if (config == NULL) return;

    Clay_ElementId field_id = CLAY_SIDI(CLAY_STRING("NumericInput"), config->uid);
    bool hovered = Clay_PointerOver(field_id);

    if (hovered && (Clay_GetPointerState().state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME))
    {
        if (focused_uid != 0 && (focused_uid != config->uid))
        {
            if (last_config != NULL)
            {
                numeric_input_commit(last_config);
                last_config->focused = false;
            }
        }
        config->focused = true;
        last_config = config;
        focused_uid = config->uid;
    }
    else if (!hovered && (Clay_GetPointerState().state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) && config->focused)
    {
        numeric_input_commit(config);
        config->focused = false;
        focused_uid = 0;
    }

    if (config->focused) numeric_input_handle_keyboard(config);

    Clay_Color border_color = config->focused ? get_color(APP_COLOR_INPUT_FOCUS) : get_color(APP_COLOR_PANEL_OUTLINE);

    CLAY(CLAY_IDI("NumericInput", config->uid),
    {
        .layout = 
        { 
            .sizing = { .width = CLAY_SIZING_FIXED(config->calculated_width), .height = CLAY_SIZING_FIXED(NUMERIC_INPUT_HEIGHT)},
            .padding = CLAY_PADDING_ALL(HALF_PADDING),
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = get_color(APP_COLOR_ITEM_BACKGROUND),
        .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS),
        .border = { .color = border_color, .width = CLAY_BORDER_OUTSIDE(NUMERIC_INPUT_BORDER_WIDTH) }
    })
    {
        CLAY(CLAY_IDI("NumericInputSpacer", config->uid),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}

        Clay_String text = 
        { 
            .chars = config->text_buffer, 
            .length = (int32_t)strlen(config->text_buffer), 
            .isStaticallyAllocated = false 
        };
        CLAY_TEXT(text, 
        { 
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE), 
            .fontSize = get_font_size_int(BASE_FONT_SIZE), 
            .textColor = get_color(APP_COLOR_TEXT) 
        });

        if (config->focused)
        {
            uint64_t now_ms = (uint64_t)(GetTime() * 1000.0);
            uint64_t phase_ms = (now_ms - config->last_keypress_frame_time) % (uint64_t)(NUMERIC_INPUT_BLINK_PERIOD_S * 1000.0f);
            bool cursor_visible = phase_ms < (uint64_t)(NUMERIC_INPUT_BLINK_PERIOD_S * 500.0f);

            CLAY(CLAY_ID("NumericInputCursor"),
            {
                .layout = 
                { 
                    .sizing = 
                    { 
                        .width = CLAY_SIZING_FIXED(NUMERIC_INPUT_CURSOR_WIDTH), 
                        .height = CLAY_SIZING_GROW(0) 
                    } 
                },
                .backgroundColor = cursor_visible ? get_color(APP_COLOR_INPUT_FOCUS) : (Clay_Color) { 0,0,0,0 }
            }) {}
        }
    }
    return;
}