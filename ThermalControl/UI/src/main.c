/* Header */
#include "main.h"

/* Defines */
#define WINDOW_ICON_SIZE_COUNT      4

/* Fonts */
static Font fonts[TOTAL_FONT_SLOTS];

/* Static local variables */
static bool close_window = false;

/* Static function definitions */
static void handle_clay_errors(Clay_ErrorData error_data)
{
    printf("Clay Error: %s\n", error_data.errorText.chars);
    return;
}

static void close(void)
{
    close_window = true;
    return;
}

int main(void)
{
    /* Initialize clay with raylib */
    Clay_Raylib_Initialize(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE, FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    /* Initialize fonts */
    load_font_family(FAMILY_OPEN_SANS, "resources/fonts/OpenSans.ttf", fonts, TOTAL_FONT_SLOTS);
    load_font_family(FAMILY_SEGOE, "resources/fonts/segoeui.ttf", fonts, TOTAL_FONT_SLOTS);
	temp_graph_set_label_font(fonts[font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE)]);
    numeric_input_set_font(fonts[font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE)]);

    /* Claim minimum memory for clay */
    uint32_t total_memory_size = Clay_MinMemorySize();
    Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(total_memory_size, malloc(total_memory_size));

    /* Initialize clay itself */
    Clay_Initialize(clay_memory, (Clay_Dimensions) { .width = (float)SCREEN_WIDTH, .height = (float)SCREEN_HEIGHT }, (Clay_ErrorHandler) { .errorHandlerFunction = handle_clay_errors, .userData = 0 });
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    /* Initialize UI language */
    lang_init(LANGUAGE_ID_DE);

    /* Initialize error handler */
    error_window_init();

    /* Initialize bridge interfaces */
    bridge_connection_init();
	bridge_flash_init();
    bridge_control_init();

    /* Initialize dropdown */
    dropdown_init();

    /* Initialize flash window */
    flash_window_init();

    /* Initialize window for warning about importing */
    warning_import_window_init();

    /* Initialize topbar */
    topbar_init();

    /* Initialize graph */
    temp_graph_init();

    /* Initialize console */
    console_init();

    /* Initialize log queue and register pull function */
    bridge_log_queue_init();
    console_register_pull_function(bridge_log_queue_pull);

    /* Initialize control panel */
    control_panel_init();

    /* Initialize serial port control */
    serial_port_init();

    /* Worker thread */
    worker_com_list_start();
	worker_sensor_start();
    worker_flash_start();

    /* UI loop */
    bool icon_set = false;
    while (!WindowShouldClose() && !close_window)
    {
        /* Set dimensions */
        Clay_SetLayoutDimensions((Clay_Dimensions) { .width = (float)GetScreenWidth(), .height = (float)GetScreenHeight() });

        /* Get current mouse position */
        Vector2 mouse_position = GetMousePosition();
        Clay_SetPointerState((Clay_Vector2) { .x = mouse_position.x, .y = mouse_position.y }, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { GetMouseWheelMoveV().x, GetMouseWheelMoveV().y }, GetFrameTime());

        /* Start of current UI layout */
        Clay_BeginLayout();

        CLAY(CLAY_ID("Root"),
            {
                .layout =
                {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                    .childGap = STANDARD_PADDING
                },
                .backgroundColor = get_color(APP_COLOR_BACKGROUND)
            })
        {
            // Obere Menueleiste
            topbar_render();

            // Restlicher Bereich, aktuell leer
            CLAY(CLAY_ID("Content"),
                {
                    .layout =
                    {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = STANDARD_PADDING
                    },
                    .backgroundColor = get_color(APP_COLOR_BACKGROUND),

                })
            {
                control_panel_render();

                CLAY(CLAY_ID("MainArea"),
                {
                    .layout =
                    {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childGap = STANDARD_PADDING
                    },
                    .backgroundColor = get_color(APP_COLOR_BACKGROUND),
                })
                {
                    temp_graph_render();
                    console_render();
                }
            }
        }

        /* Renders error window if an error is active, else do nothing */
        error_window_render();

        /* Renders flash window if visible */
        flash_window_render();

        /* Renders warning window for importing, if import & sensor is connected */
        warning_import_window_render();

        /* Closes dropdown menus if clicked outside */
        dropdown_handle_click_outside();

        /* Get render commands & pipe it to raylib */
        Clay_RenderCommandArray render_commands = Clay_EndLayout(GetFrameTime());
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Clay_Raylib_Render(render_commands, fonts);
        EndDrawing();

        if (!icon_set)
        {
            Image icons[WINDOW_ICON_SIZE_COUNT];
            icons[0] = LoadImage("resources/images/app_icon_16.png");
            icons[1] = LoadImage("resources/images/app_icon_32.png");
            icons[2] = LoadImage("resources/images/app_icon_48.png");
            icons[3] = LoadImage("resources/images/app_icon_256.png");

            SetWindowIcons(icons, WINDOW_ICON_SIZE_COUNT);

            for (int icon_index = 0; icon_index < WINDOW_ICON_SIZE_COUNT; icon_index++) UnloadImage(icons[icon_index]);

            PollInputEvents();
            icon_set = true;
        }
    }

    /* Close backend tasks */
    worker_com_list_stop();
    worker_sensor_stop();
    worker_flash_stop();

    Clay_Raylib_Close();
    return 0;
}