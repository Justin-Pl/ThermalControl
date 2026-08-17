/* Header */
#include "data_import.h"

/* Defines */
#define DATA_IMPORT_MAX_PATH                256

/* Static function definitions */
static void log_import(const bool success, const char* what, const char* path)
{
    /* Check parameters */
    if (what == NULL) return;
    if (path == NULL) return;

    if (success)
    {
        char message[512];
        snprintf(message, sizeof(message), "Import successfully %s from path: %s", what, path);
        bridge_log_queue_push(message);
        return;
    }

    bridge_publish_error(ERROR_EXPORT_FAILED, "Import failed!");
    return;
}

static bool parse_csv_line(const char* line, graph_point_t* out_point)
{
    unsigned long long utc_ms;
    float relative_s;
    float temp;
    float pwm;

    int matched = sscanf(line, "%llu,%f,%f,%f", &utc_ms, &relative_s, &temp, &pwm);
    if (matched != 4) return false;

    out_point->timestamp_ms = (uint64_t)utc_ms;
    out_point->temp = temp;
    out_point->pwm = pwm;
    return true;
}

static void graph_log_imported_point(const graph_point_t* new_point)
{
    /* Check if point is valid */
    if (new_point == NULL) return;

    char point_log[64];
    snprintf(point_log, sizeof(point_log), "Imported point! Temp.: %.2f C, PWM: %.0f%%", new_point->temp, new_point->pwm);

    bridge_log_queue_push(point_log);

    return;
}

static bool graph_import(const char* filepath)
{
    /* Check arguments */
    if (filepath == NULL) return false;

    /* Open file */
    FILE* file = fopen(filepath, "r");
    if (file == NULL) return false;

    /* Clear old data history */
    temp_graph_clear();

    /* Iterate through every file in .csv */
    char line[128];
    size_t point_count = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (line[0] == '#') continue;

        graph_point_t point;
        if (parse_csv_line(line, &point))
        {
            temp_graph_import_points(&point, 1);  
            graph_log_imported_point(&point);
            point_count++;
        }
    }

    fclose(file);

    char log_msg[64];
    if (!point_count)
    {
        strncpy(log_msg, "No points found for import!", sizeof(log_msg));
    }
    else
    {
        snprintf(log_msg, sizeof(log_msg), "%zu points successfully imported!", (unsigned)point_count);
    }
    bridge_log_queue_push(log_msg);

    return point_count ? true : false;
}

static bool control_config_import(const char* filepath, bridge_control_config_t* out_config)
{
    if ((filepath == NULL) || (out_config == NULL)) return false;

    FILE* file = fopen(filepath, "r");
    if (file == NULL) return false;

    bridge_control_config_t result = *out_config;

    char line[128];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (line[0] == '#') continue;

        char key[64];
        char value[64];
        if (sscanf(line, "%63[^=]=%63s", key, value) != 2) continue;

        if (strcmp(key, "mode") == 0) result.mode = (bridge_control_mode_t)atoi(value);
        else if (strcmp(key, "manual_pwm_percent") == 0) result.manual_pwm_percent = (float)atof(value);
        else if (strcmp(key, "two_point_setpoint_c") == 0) result.two_point_setpoint_c = (float)atof(value);
        else if (strcmp(key, "two_point_hysteresis_c") == 0) result.two_point_hysteresis_c = (float)atof(value);
        else if (strcmp(key, "pid_setpoint_c") == 0) result.pid_setpoint_c = (float)atof(value);
        else if (strcmp(key, "pid_kp") == 0) result.pid_kp = (float)atof(value);
        else if (strcmp(key, "pid_ki") == 0) result.pid_ki = (float)atof(value);
        else if (strcmp(key, "pid_kd") == 0) result.pid_kd = (float)atof(value);
        else if (strcmp(key, "pid_p_enabled") == 0) result.pid_p_enabled = (atoi(value) != 0);
        else if (strcmp(key, "pid_i_enabled") == 0) result.pid_i_enabled = (atoi(value) != 0);
        else if (strcmp(key, "pid_d_enabled") == 0) result.pid_d_enabled = (atoi(value) != 0);
    }

    fclose(file);
    *out_config = result;
    return true;
}

static bool config_import(const char* filepath)
{
    if (filepath == NULL) return false;

    bridge_control_config_t imported_config;
    if (!bridge_get_control_config(&imported_config)) return false;

    if (!control_config_import(filepath, &imported_config)) return false;

    control_panel_apply_imported_config(&imported_config);
    bridge_publish_control_config(&imported_config);

    return true;
}

static bool console_import(const char* filepath)
{
    if (filepath == NULL) return false;

    FILE* file = fopen(filepath, "r");
    if (file == NULL) return false;

    console_clear();

    char line[CONSOLE_MAX_MESSAGE_LENGTH];
    size_t imported_count = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        size_t len = strlen(line);
        while ((len > 0) && ((line[len - 1] == '\n') || (line[len - 1] == '\r')))
        {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        console_import_raw_line(line);
        imported_count++;
    }

    fclose(file);
    return imported_count ? true : false;
}

void data_import_graph_csv(void)
{
    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_IMPORT_DATA_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_CSV_FILES)->chars,
        .filter_extension = "csv",
        .default_filename = NULL
    };

    char path[DATA_IMPORT_MAX_PATH];
    if (!show_open_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = graph_import(path);
    log_import(success, "graph", path);
    return;
}

void data_import_control_config(void)
{
    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_IMPORT_CONFIG_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_CFG_FILES)->chars,
        .filter_extension = "cfg",
        .default_filename = NULL
    };

    char path[DATA_IMPORT_MAX_PATH];
    if (!show_open_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = config_import(path);
    log_import(success, "config", path);
    return;
}

void data_import_console_txt(void)
{
    file_dialog_config_t dialog_config =
    {
        .dialog_title = lang_get(STRING_ID_IMPORT_CONSOLE_DIALOG_TITLE)->chars,
        .filter_label = lang_get(STRING_ID_TXT_FILES)->chars,
        .filter_extension = "txt",
        .default_filename = NULL
    };

    char path[DATA_IMPORT_MAX_PATH];
    if (!show_open_dialog(&dialog_config, path, sizeof(path))) return;

    bool success = console_import(path);
    log_import(success, "console", path);
    return;
}