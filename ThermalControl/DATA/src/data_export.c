/* Header */
#include "data_export.h"

/* Defines */
#define GRAPH_EXPORT_REFERENCE_HEIGHT       500.0f
#define DATA_EXPORT_MAX_PATH                256

/* Static function definitions */
static void log_export(const bool success, const char* what, const char* path)
{
    /* Check parameters */
    if (what == NULL) return;
    if (path == NULL) return;

    if (success)
    {
        char message[512];
        snprintf(message, sizeof(message), "Exported successfully %s to path: %s", what, path);
        bridge_log_queue_push(message);
        return;
    }

    bridge_publish_error(ERROR_EXPORT_FAILED, "Export failed!");
    return;
}

static bool export_graph_to_png(const char* filepath, int export_width, int export_height)
{
    if ((filepath == NULL) || (export_width <= 0) || (export_height <= 0)) return false;

    RenderTexture2D target = LoadRenderTexture(export_width, export_height);
    if (target.id == 0) return false;

    BeginTextureMode(target);
    ClearBackground(get_color_raylib(APP_COLOR_BACKGROUND));

    float scale_factor = (float)export_height / GRAPH_EXPORT_REFERENCE_HEIGHT;
    Clay_BoundingBox export_box = { 0.0f, 0.0f, (float)export_width, (float)export_height };
    temp_graph_draw_for_export(export_box, scale_factor);

    EndTextureMode();

    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);

    bool success = ExportImage(image, filepath);

    UnloadImage(image);
    UnloadRenderTexture(target);

    return success;
}

static bool control_config_export(const char* filepath, const bridge_control_config_t* config)
{
    if ((filepath == NULL) || (config == NULL)) return false;

    FILE* file = fopen(filepath, "w");
    if (file == NULL) return false;

    fprintf(file, "# ThermalControl Reglerkonfiguration\n");
    fprintf(file, "mode=%d\n", (int)config->mode);
    fprintf(file, "manual_pwm_percent=%.2f\n", config->manual_pwm_percent);
    fprintf(file, "two_point_setpoint_c=%.2f\n", config->two_point_setpoint_c);
    fprintf(file, "two_point_hysteresis_c=%.2f\n", config->two_point_hysteresis_c);
    fprintf(file, "pid_setpoint_c=%.2f\n", config->pid_setpoint_c);
    fprintf(file, "pid_kp=%.2f\n", config->pid_kp);
    fprintf(file, "pid_ki=%.2f\n", config->pid_ki);
    fprintf(file, "pid_kd=%.2f\n", config->pid_kd);
    fprintf(file, "pid_p_enabled=%d\n", config->pid_p_enabled ? 1 : 0);
    fprintf(file, "pid_i_enabled=%d\n", config->pid_i_enabled ? 1 : 0);
    fprintf(file, "pid_d_enabled=%d\n", config->pid_d_enabled ? 1 : 0);

    fclose(file);
    return true;
}

/* Function definitions */
void data_export_graph_csv(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id; 
    (void)user_data;
    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

    file_dialog_config_t dialog_config = 
    { 
        .dialog_title = lang_get(STRING_ID_EXPORT_GRAPH_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_CSV_FILES)->chars,
        .filter_extension = "csv", 
        .default_filename = lang_get(STRING_ID_CSV_FILENAME)->chars
    };

    char path[DATA_EXPORT_MAX_PATH];
    if (!show_save_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = temp_graph_export_points_to_csv(path);
    log_export(success, "graph", path);
    return;
}

void data_export_graph_png(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id; 
    (void)user_data;
    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_EXPORT_GRAPH_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_PNG_FILES)->chars,
        .filter_extension = "png",
        .default_filename = lang_get(STRING_ID_PNG_FILENAME)->chars
    };

    char path[DATA_EXPORT_MAX_PATH];
    if (!show_save_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = export_graph_to_png(path, 1920, 1080);
    log_export(success, "graph", path);
    return;
}

void data_export_console_txt(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;
    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_EXPORT_CONSOLE_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_TXT_FILES)->chars,
        .filter_extension = "txt",
        .default_filename = lang_get(STRING_ID_TXT_FILENAME)->chars
    };

    char path[DATA_EXPORT_MAX_PATH];
    if (!show_save_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = console_export_to_txt(path);
    log_export(success, "console", path);
    return;
}

void data_export_control_config_cfg(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;
    if (data.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) return;

    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_EXPORT_CONFIG_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_CFG_FILES)->chars,
        .filter_extension = "cfg",
        .default_filename = lang_get(STRING_ID_CFG_FILENAME)->chars
    };

    char path[DATA_EXPORT_MAX_PATH];
    if (!show_save_dialog(&dialog_config, path, sizeof(path))) return;

    bridge_control_config_t current_config;
    if (!bridge_get_control_config(&current_config)) return;

    bool success = control_config_export(path, &current_config);
    log_export(success, "control", path);
    return;
}