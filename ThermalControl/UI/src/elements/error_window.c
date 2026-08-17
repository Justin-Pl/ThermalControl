/* Header */
#include "elements/error_window.h"

/* Defines */
#define ERROR_WINDOW_WIDTH                      400
#define ERROR_WINDOW_OUTLINE_WIDTH              2
#define ERROR_WINDOW_BACKGROUND_TRANSPARENCY    80

/* Static local variables */
static bool error_active = false;
static error_entry_t current_error = { .code = ERROR_NONE, .message = "" };

/* Function definitions */
void error_window_init(void)
{
	bridge_error_init();        
    memset(&current_error, 0, sizeof(current_error));
	return;
}

void error_window_render(void)
{
    if (!error_active)
    {
        if (!bridge_pull_error(&current_error)) return;
		error_active = true;
    }
	
    Clay_String error_message = { .isStaticallyAllocated = true, .length = (int32_t)strlen(current_error.message), .chars = current_error.message };

    CLAY(CLAY_ID("ErrorModalOverlay"),
    {
        .layout =
        {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = { .a = ERROR_WINDOW_BACKGROUND_TRANSPARENCY },
        .floating =
        {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex = INT16_MAX
        }
    })
    {
        CLAY(CLAY_ID("ErrorModalBox"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_FIXED(ERROR_WINDOW_WIDTH) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = STANDARD_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS),
            .border = {.color = get_color(APP_COLOR_PANEL_OUTLINE), .width = CLAY_BORDER_OUTSIDE(ERROR_WINDOW_OUTLINE_WIDTH) }
        })
        {
            CLAY_TEXT(error_message,
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });

            CLAY(CLAY_ID("ErrorModalCloseButton"),
            {
                .layout = {.padding = CLAY_PADDING_ALL(HALF_PADDING) },
                .backgroundColor = Clay_Hovered() ? get_color(APP_COLOR_HOVER) : get_color(APP_COLOR_ITEM_BACKGROUND),
                .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
            })
            {
                if (Clay_Hovered() && Clay_GetPointerState().state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
                {
                    error_active = false;
                }
                CLAY_TEXT(get_label(STRING_ID_OK),
                {
                    .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                    .fontSize = get_font_size_int(BASE_FONT_SIZE),
                    .textColor = get_color(APP_COLOR_TEXT)
                });
            }
        }
    }

	return;
}