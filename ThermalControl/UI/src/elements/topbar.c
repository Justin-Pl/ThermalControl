/* Header */
#include "elements/topbar.h"

/* Defines */
#define FILE_SAVE_EXPORT_GRAPH_PNG      0
#define FILE_SAVE_EXPORT_GRAPH_CSV      1
#define FILE_SAVE_EXPORT_CONSOLE        3
#define GRAPH_CLEAR                     0
#define GRAPH_ENABLE_AUTO_FOLLOW        1
#define GRAPH_TOGGLE_SAMPLING           2

/* Makro */
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))

/* Images */
static Texture2D topbar_logo;

/* Function callbacks */
static void change_lang_de(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) lang_set_language(LANGUAGE_ID_DE);
    return;
}

static void change_lang_en(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) lang_set_language(LANGUAGE_ID_EN);
    return;
}

static void change_theme_dark(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) set_color_palette(COLOR_PALETTE_DARK);
    return;
}

static void change_theme_light(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) set_color_palette(COLOR_PALETTE_LIGHT);
    return;
}

static void open_flash_window(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) flash_window_show();
    return;
}

static void import_wrapper(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    import_callback_t callback = (import_callback_t)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        connection_info_t connect_info;
        if (!bridge_get_connection_info(&connect_info)) return;

        if (connect_info.connected)
        {
            warning_import_window_show(callback);
            return;
        }
        else
        {
            if (callback != NULL) callback();
        }
    }
    return;
}

/* Dropdown menu definitions */
/* File menu */
static dropdown_item_t file_open_submenu_items[] =
{
    { .label = STRING_ID_FILE_OPEN_GRAPH,  .callback = import_wrapper, .user_data = data_import_graph_csv },
    { .label = STRING_ID_FILE_OPEN_CONFIG, .callback = import_wrapper, .user_data = data_import_control_config },
    { .label = STRING_ID_FILE_OPEN_CONSOLE, .callback = import_wrapper, .user_data = data_import_console_txt }
};

static dropdown_item_t file_save_submenu_items[] =
{
    {.label = STRING_ID_FILE_SAVE_GRAPH_PNG,  .callback = data_export_graph_png, .disabled = true },
    {.label = STRING_ID_FILE_SAVE_GRAPH_DATA, .callback = data_export_graph_csv, .disabled = true },
    {.label = STRING_ID_FILE_SAVE_CONFIG, .callback = data_export_control_config_cfg },
    {.label = STRING_ID_FILE_SAVE_CONSOLE, .callback = data_export_console_txt, .disabled = true }
};

static dropdown_item_t file_menu_items[] =
{
    { .label = STRING_ID_FILE_OPEN, .callback = NULL, .children = file_open_submenu_items, .child_count = ARRAY_SIZE(file_open_submenu_items) },
    { .label = STRING_ID_FILE_SAVE, .callback = NULL, .children = file_save_submenu_items, .child_count = ARRAY_SIZE(file_save_submenu_items)},
    { .label = STRING_ID_FILE_EXIT, .callback = NULL, .children = NULL, .child_count = 0 },
};

/* View menu */
static dropdown_item_t view_style_submenu_items[] =
{
    { .label = STRING_ID_VIEW_STYLE_DARK,  .callback = change_theme_dark,  .disabled = true },
    { .label = STRING_ID_VIEW_STYLE_LIGHT, .callback = change_theme_light, .disabled = true },
};

static dropdown_item_t view_lang_submenu_items[] =
{
    { .label = STRING_ID_VIEW_LANG_DE, .callback = change_lang_de, .disabled = true },
    { .label = STRING_ID_VIEW_LANG_EN, .callback = change_lang_en, .disabled = true },
};

static dropdown_item_t view_menu_items[] =
{
    { .label = STRING_ID_VIEW_STYLE, .callback = NULL, .children = view_style_submenu_items, .child_count = ARRAY_SIZE(view_style_submenu_items) },
    { .label = STRING_ID_VIEW_LANG,  .callback = NULL, .children = view_lang_submenu_items, .child_count = ARRAY_SIZE(view_lang_submenu_items) },
};

/* Console menu */
static dropdown_item_t console_menu_items[] =
{
    { .label = STRING_ID_CLEAR, .callback = console_handle_clear_button_interaction, .disabled = true },
    { .label = STRING_ID_AUTO_FOLLOW, .callback = console_handle_auto_follow_button_interaction, .disabled = true },
};

/* Graph menu */
static dropdown_item_t graph_menu_items[] =
{
    { .label = STRING_ID_CLEAR, .callback = temp_graph_handle_clear_button_interaction, .disabled = true },
    { .label = STRING_ID_AUTO_FOLLOW, .callback = temp_graph_handle_auto_follow_button_interaction, .disabled = true },
    { .label = STRING_ID_ENABLE_SAMPLING, .callback = temp_graph_enable_sampling }
};

/* Device menu */
static dropdown_item_t device_menu_items[] =
{
    { .label = STRING_ID_FLASH_FIRMWARE, .callback = open_flash_window, .disabled = true },
};

/* Function definition */
void topbar_init(void)
{
    /* Load images */
    topbar_logo = LoadTexture("resources/images/logo.png");

    /* Register dropdown menus */
    dropdown_register_menu(file_menu_items, ARRAY_SIZE(file_menu_items));
    dropdown_register_menu(view_menu_items, ARRAY_SIZE(view_menu_items));
    dropdown_register_menu(console_menu_items, ARRAY_SIZE(console_menu_items));
    dropdown_register_menu(graph_menu_items, ARRAY_SIZE(graph_menu_items));
    dropdown_register_menu(device_menu_items, ARRAY_SIZE(device_menu_items));
    return;
}

void topbar_render(void)
{
    file_save_submenu_items[FILE_SAVE_EXPORT_GRAPH_PNG].disabled = temp_graph_get_data_count() > 1 ? false : true;
    file_save_submenu_items[FILE_SAVE_EXPORT_GRAPH_CSV].disabled = temp_graph_get_data_count() > 1 ? false : true;
    file_save_submenu_items[FILE_SAVE_EXPORT_CONSOLE].disabled = console_get_line_counter() ? false : true;

    view_style_submenu_items[0].disabled = get_current_color_palette() == COLOR_PALETTE_DARK ? true : false;
    view_style_submenu_items[1].disabled = get_current_color_palette() == COLOR_PALETTE_LIGHT ? true : false;

    view_lang_submenu_items[0].disabled = lang_get_current_language() == LANGUAGE_ID_DE ? true : false;
    view_lang_submenu_items[1].disabled = lang_get_current_language() == LANGUAGE_ID_EN ? true : false;

    console_menu_items[0].disabled = console_get_line_counter() ? false : true;
    console_menu_items[1].disabled = console_get_auto_scroll_state() ? true : false;

    graph_menu_items[GRAPH_CLEAR].disabled = temp_graph_get_data_count() ? false : true;
    graph_menu_items[GRAPH_ENABLE_AUTO_FOLLOW].disabled = temp_graph_get_auto_scroll_state() ? true : false;
    graph_menu_items[GRAPH_TOGGLE_SAMPLING].label = temp_graph_sampling_enabled() ? STRING_ID_DISABLE_SAMPLING : STRING_ID_ENABLE_SAMPLING;

    /* Get current connection info, thread safe */
    /* Set device options depending on connection */
    connection_info_t connect_info;
    bridge_get_connection_info(&connect_info);
    for (size_t menu_index = 0; menu_index < ARRAY_SIZE(device_menu_items); menu_index++)
    {
        device_menu_items[menu_index].disabled = connect_info.connected ? false : true;
    }


    CLAY(CLAY_ID("TopBar"),
    {
        .layout =
        {
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .childGap = MIN_PADDING,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER, .x = CLAY_ALIGN_X_LEFT },
        },
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND)
    })
    {
        CLAY(CLAY_ID("TopBarLogoWrapper"),
        {
            .layout =
            {
                .padding = {.top = HALF_PADDING, .bottom = HALF_PADDING, .left = HALF_PADDING, .right = 0 },
				.childGap = MIN_PADDING,
				.childAlignment = { .y = CLAY_ALIGN_Y_CENTER, .x = CLAY_ALIGN_X_LEFT }
            }
        })
        {
            CLAY(CLAY_ID("TopBarLogo"),
            {
                .layout =
                {
                    .sizing = {.width = CLAY_SIZING_FIXED(24), .height = CLAY_SIZING_FIXED(24) },
                    
                },
                .image = { .imageData = &topbar_logo }
            }) {}
            dropdown_render_menu(get_label(STRING_ID_MENU_FILE), file_menu_items, ARRAY_SIZE(file_menu_items), false, NULL);
            dropdown_render_menu(get_label(STRING_ID_MENU_VIEW), view_menu_items, ARRAY_SIZE(view_menu_items), false, NULL);
            dropdown_render_menu(get_label(STRING_ID_MENU_CONSOLE), console_menu_items, ARRAY_SIZE(console_menu_items), false, NULL);
            dropdown_render_menu(get_label(STRING_ID_MENU_GRAPH), graph_menu_items, ARRAY_SIZE(graph_menu_items), false, NULL);
            dropdown_render_menu(get_label(STRING_ID_DEVICE), device_menu_items, ARRAY_SIZE(device_menu_items), false, NULL);
        }
    }
    return;
}