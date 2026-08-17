/* Header */
#include "elements/warning_import.h"

/* Defines */
#define WARNING_IMPORT_WINDOW_WIDTH						400
#define WARNING_IMPORT_OUTLINE_WIDTH					2
#define WARNING_IMPORT_BACKGROUND_TRANSPARENCY			80
#define WARNING_IMPORT_Z_INDEX							INT16_MAX / 2

/* Static local variables */
static bool window_visible = false;
static bool wait_for_closing = false;
static button_config_t start_button;
static button_config_t cancel_button;
static import_callback_t callback = NULL;

/* Static function definitions */
static void warning_import_render_buttons(bool disable_cancel, bool disable_flash)
{
    CLAY(CLAY_ID("WARNING_IMPORT_BUTTON_WRAPPER"),
    {
        .layout =
        {
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .childGap = STANDARD_PADDING
        },
    })
    {
        start_button.enabled = !disable_flash ? true : false;
        button_render(&start_button);

        CLAY(CLAY_ID("WarningImportButtonSpacer"),
            {
                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0) } }
            }) {
        }

        cancel_button.enabled = !disable_cancel ? true : false;
        button_render(&cancel_button);
    }
    return;
}

static void handle_cancel(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        window_visible = false;
    }
    return;
}

static void handle_import(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        bridge_publish_connection_request(0, 0, false, false);
        wait_for_closing = true;
    }
    return;
}

/* Function definitions */
void warning_import_window_init(void)
{
    /* Initialize buttons */
    memset(&start_button, 0, sizeof(start_button));
    button_init(&start_button);
    start_button.labels.enabled_label = STRING_ID_IMPORT;
    start_button.labels.disabled_label = STRING_ID_IMPORT;
    start_button.enabled = true;
    start_button.is_toggle_button = false;
    start_button.callback = handle_import;

    memset(&cancel_button, 0, sizeof(cancel_button));
    button_init(&cancel_button);
    cancel_button.labels.enabled_label = STRING_ID_CANCEL;
    cancel_button.labels.disabled_label = STRING_ID_CANCEL;
    cancel_button.enabled = true;
    cancel_button.is_toggle_button = false;
    cancel_button.callback = handle_cancel;

    return;
}

void warning_import_window_show(const import_callback_t import_callback)
{
    callback = import_callback;
    window_visible = true;
    return;
}

void warning_import_window_render(void)
{
    if (!window_visible) return;

    start_button.enabled = wait_for_closing ? false : true;
    cancel_button.enabled = wait_for_closing ? false : true;

    if (wait_for_closing)
    {
        connection_info_t current_info = { .connected = false };
        bridge_get_connection_info(&current_info);

        if (!current_info.connected)
        {
            if (callback != NULL) callback();
            wait_for_closing = false;
            window_visible = false;
        }
        else
        {
            bridge_publish_connection_request(0, 0, false, false);
        }
    }

    CLAY(CLAY_ID("WarningImportModalOverlay"),
    {
        .layout =
        {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = {.a = WARNING_IMPORT_BACKGROUND_TRANSPARENCY },
        .floating =
        {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex = WARNING_IMPORT_Z_INDEX
        }
    })
    {
        CLAY(CLAY_ID("WarningImportTopbar"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(WARNING_IMPORT_WINDOW_WIDTH), .height = CLAY_SIZING_FIXED(50) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childGap = STANDARD_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = {.topLeft = STANDARD_RADIUS, .topRight = STANDARD_RADIUS },
            .border = {.color = get_color(APP_COLOR_PANEL_OUTLINE), .width = {.left = WARNING_IMPORT_OUTLINE_WIDTH, .right = WARNING_IMPORT_OUTLINE_WIDTH, .top = WARNING_IMPORT_OUTLINE_WIDTH } }
        })
        {
            CLAY_TEXT(get_label(STRING_ID_IMPORT_WARNING),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });
        }
        CLAY(CLAY_ID("FlashBox"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(WARNING_IMPORT_WINDOW_WIDTH) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = STANDARD_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = {.bottomLeft = STANDARD_RADIUS, .bottomRight = STANDARD_RADIUS },
            .border = {.color = get_color(APP_COLOR_PANEL_OUTLINE), .width = CLAY_BORDER_OUTSIDE(WARNING_IMPORT_OUTLINE_WIDTH) }
        })
        {
            CLAY_TEXT(get_label(STRING_ID_IMPORT_WARNING_TEXT),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT_WARNING)
            });
            warning_import_render_buttons(false, false);
        }
    }

    return;
}