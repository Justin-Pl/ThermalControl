/* Header */
#include "elements/flash_window.h"

/* Defines */
#define FLASH_WINDOW_WIDTH                      400
#define FLASH_WINDOW_OUTLINE_WIDTH              2
#define FLASH_WINDOW_BACKGROUND_TRANSPARENCY    80
#define FLASH_WINDOW_Z_INDEX				    INT16_MAX / 2

/* Static local variables */
static bool flash_window_visible = false;
static button_config_t flash_start_button;
static button_config_t flash_cancel_button;
static dropdown_item_t firmware_items[FIRMWARE_FILES_MAX];
static size_t firmware_item_count = 0;
static char selected_firmware_file[FIRMWARE_FILE_NAME_MAX] = { 0 };
static const dropdown_sizing_t menu_sizing_grow =
{
    .button_width = CLAY_SIZING_GROW(0),
    .panel_width = CLAY_SIZING_GROW(0),
    .submenu_width = CLAY_SIZING_GROW(0),
	.base_z_index = FLASH_WINDOW_Z_INDEX
};

/* Static function definitions */
static void flash_render_buttons(bool disable_cancel, bool disable_flash)
{
    CLAY(CLAY_ID("FLASH_BUTTON_WRAPPER"),
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
		flash_start_button.enabled = strlen(selected_firmware_file) && !disable_flash ? true : false;
        button_render(&flash_start_button);

        CLAY(CLAY_ID("FlashButtonSpacer"),
        {
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0) } }
        }) {}

		flash_cancel_button.enabled = !disable_cancel ? true : false;
        button_render(&flash_cancel_button);
    }
    return;
}

static void flash_render_firmware_list(bool disable)
{
    CLAY(CLAY_ID("FIRMWARE_LIST_WRAPPER"),
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
        CLAY(CLAY_ID("FIRMWARE_LIST_LABEL"),
        {
            .layout =
            {
                .sizing = {.width = (FLASH_WINDOW_WIDTH / 2) - (STANDARD_PADDING * 2)}
            },
        })
        {
            CLAY_TEXT(get_label(STRING_ID_FIRMWARE_FILE),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });
        }

        static Clay_String current_firmware_label;
        if (strlen(selected_firmware_file))
        {
            current_firmware_label.chars = selected_firmware_file;
            current_firmware_label.length = strlen(selected_firmware_file);
        }
        else
        {
            current_firmware_label.chars = "???";
            current_firmware_label.length = strlen("???");
        }
        dropdown_render_menu(current_firmware_label, firmware_items, firmware_item_count, disable, &menu_sizing_grow);
    }
	return;
}

static void flash_render_progress(void)
{
	bridge_flash_progress_t progress = { 0 };
    float percentage = 0.01f;
    bool flashing = false;
    Clay_Color backgroundColor = get_color(APP_COLOR_BUTTON_BACKGROUND_DISABLED);
    if (bridge_get_flash_progress(&progress))
    {
		percentage = (float)progress.progress_percentage / 100.0f;
		flashing = (progress.state != BRIDGE_FLASH_STATE_IDLE);
		backgroundColor = flashing ? get_color(APP_COLOR_ITEM_BACKGROUND) : get_color(APP_COLOR_BUTTON_BACKGROUND_DISABLED);
    }
    if (percentage < 0.05f) percentage = 0.05f;

    CLAY_TEXT(get_label(STRING_ID_PROGRESS),
    {
        .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
        .fontSize = get_font_size_int(BASE_FONT_SIZE),
        .textColor = get_color(APP_COLOR_TEXT)
    });

    CLAY(CLAY_ID("FLASH_PROGRESS_WRAPPER"),
    {
        .layout =
        {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(24) },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .padding = CLAY_PADDING_ALL(MIN_PADDING)
        },
		.backgroundColor = backgroundColor,
        .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
    })
    {
        if (flashing)
        {
            CLAY(CLAY_ID("FLASH_PROGRESS_BAR"),
            {
                .layout =
                {
                    .sizing = { .width = CLAY_SIZING_PERCENT(percentage), .height = CLAY_SIZING_GROW(0) }
                },
                .backgroundColor = get_color(APP_COLOR_PROGRESS_TRACK),
                .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
            }) {}
        }
    }

    return;
}

static void flash_window_select_firmware(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    int index = (int)(intptr_t)user_data;
    strncpy(selected_firmware_file, firmware_items[index].raw_label, sizeof(selected_firmware_file) - 1);
    return;
}

static void flash_handle_cancel(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
		flash_window_visible = false;
    }
    return;
}

static void flash_handle_flashing(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
		bridge_flash_params_t params = { 0 };
        strncpy(params.firmware_file, selected_firmware_file, sizeof(params.firmware_file) - 1);
        params.firmware_file[sizeof(params.firmware_file) - 1] = '\0';
        bridge_publish_flash_request(&params);
    }
    return;
}

/* Function definitions */
void flash_window_init(void)
{
    /* Initialize firmware file list */
    memset(firmware_items, 0, sizeof(firmware_items));
	memset(selected_firmware_file, 0, sizeof(selected_firmware_file));

    firmware_file_list_t list = { 0 };
    if (firmware_scan_files(&list))
    {
        for (size_t file_index = 0; file_index < list.count; file_index++)
        {
            /* Copy file name */
            char* raw_label = firmware_items[file_index].raw_label;
			char* file_name = list.files[file_index].name;
			size_t label_size = sizeof(firmware_items[file_index].raw_label);
			strncpy(raw_label, file_name, label_size - 1);
			raw_label[label_size - 1] = '\0';
        
            /* Set dropdown item configuration */
			dropdown_item_t* current_item = &firmware_items[file_index];
			current_item->use_raw_label = true;
            current_item->disabled = false;
			current_item->callback = flash_window_select_firmware;
            current_item->user_data = (void*)(intptr_t)file_index;
        }
		firmware_item_count = list.count;
    }
    dropdown_register_menu(firmware_items, firmware_item_count);

    /* Initialize buttons */
    memset(&flash_start_button, 0, sizeof(flash_start_button));
    button_init(&flash_start_button);
    flash_start_button.labels.enabled_label = STRING_ID_FLASH_FIRMWARE;
    flash_start_button.labels.disabled_label = STRING_ID_FLASH_FIRMWARE;
    flash_start_button.enabled = true;
    flash_start_button.is_toggle_button = false;
    flash_start_button.callback = flash_handle_flashing;

    memset(&flash_cancel_button, 0, sizeof(flash_cancel_button));
    button_init(&flash_cancel_button);
    flash_cancel_button.labels.enabled_label = STRING_ID_CANCEL;
    flash_cancel_button.labels.disabled_label = STRING_ID_CANCEL;
    flash_cancel_button.enabled = true;
    flash_cancel_button.is_toggle_button = false;
    flash_cancel_button.callback = flash_handle_cancel;

	return;
}

void flash_window_show(void)
{
	flash_window_visible = true;
    bridge_flash_progress_t progress = { .state = BRIDGE_FLASH_STATE_IDLE, .progress_percentage = 0 };
    bridge_publish_flash_progress(&progress);
    return;
}

void flash_window_render(void)
{
    if (!flash_window_visible) return;

	bridge_flash_progress_t progress = { 0 };
    bool disable_dropdown = true;
    bool disable_cancel = true;
	bool disable_flash = true;
    if (bridge_get_flash_progress(&progress))
    {
        if (progress.state == BRIDGE_FLASH_STATE_IDLE)
        {
            disable_dropdown = false;
            disable_cancel = false;
            if (strlen(selected_firmware_file)) disable_flash = false;
        }
		else if ((progress.state == BRIDGE_FLASH_COMPLETED) || (progress.state == BRIDGE_FLASH_ERROR))
		{
			disable_dropdown = true;
			disable_cancel = false;
			disable_flash = true;
		}
        else
        {
			disable_dropdown = true;
			disable_cancel = true;
			disable_flash = true;
        }
    }

    CLAY(CLAY_ID("FlashingModalOverlay"),
    {
        .layout =
        {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = {.a = FLASH_WINDOW_BACKGROUND_TRANSPARENCY },
        .floating =
        {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .zIndex = FLASH_WINDOW_Z_INDEX
        }
    })
    {
        CLAY(CLAY_ID("FlashTopbar"),
        {
            .layout =
            {
                .sizing = { .width = CLAY_SIZING_FIXED(FLASH_WINDOW_WIDTH), .height = CLAY_SIZING_FIXED(50) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childGap = STANDARD_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = { .topLeft = STANDARD_RADIUS, .topRight = STANDARD_RADIUS },
            .border = {.color = get_color(APP_COLOR_PANEL_OUTLINE), .width = { .left = FLASH_WINDOW_OUTLINE_WIDTH, .right = FLASH_WINDOW_OUTLINE_WIDTH, .top = FLASH_WINDOW_OUTLINE_WIDTH } }
        })
        { 
            CLAY_TEXT(get_label(STRING_ID_FIRMWARE_UPDATER),
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
                .sizing = {.width = CLAY_SIZING_FIXED(FLASH_WINDOW_WIDTH) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = STANDARD_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = {.bottomLeft = STANDARD_RADIUS, .bottomRight = STANDARD_RADIUS },
            .border = { .color = get_color(APP_COLOR_PANEL_OUTLINE), .width = CLAY_BORDER_OUTSIDE(FLASH_WINDOW_OUTLINE_WIDTH) }
        })
        {
            flash_render_firmware_list(disable_dropdown);
            CLAY_TEXT(get_label(STRING_ID_FLASH_WARNING),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT_WARNING)
            });
            flash_render_progress();
            flash_render_buttons(disable_cancel, disable_flash);
        }
    }

	return;
}