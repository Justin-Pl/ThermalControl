/* Header */
#include "file_dialog.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

/* Extern raylib function */
extern void* GetWindowHandle(void);

/* Function definitions */
bool show_save_dialog(const file_dialog_config_t* config, char* out_path, size_t out_path_size)
{
    if ((config == NULL) || (out_path == NULL) || !out_path_size) return false;

    char file_buffer[MAX_PATH] = { 0 };
    if (config->default_filename != NULL)
    {
        strncpy(file_buffer, config->default_filename, sizeof(file_buffer) - 1);
    }

    
    static char filter_buffer[MAX_PATH];
    int offset = 0;
    offset += snprintf(&filter_buffer[offset], sizeof(filter_buffer) - offset, "%s (*.%s)", config->filter_label, config->filter_extension);
    offset += 1;
    offset += snprintf(&filter_buffer[offset], sizeof(filter_buffer) - offset, "*.%s", config->filter_extension);
    offset += 1;
    filter_buffer[offset] = '\0';   // Second null byte for termination

    OPENFILENAMEA open_filename = { 0 };
    open_filename.lStructSize = sizeof(open_filename);
    open_filename.hwndOwner = (HWND)GetWindowHandle();
    open_filename.lpstrFilter = filter_buffer;
    open_filename.lpstrFile = file_buffer;
    open_filename.nMaxFile = sizeof(file_buffer);
    open_filename.lpstrDefExt = config->filter_extension;
    open_filename.lpstrTitle = config->dialog_title;
    open_filename.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameA(&open_filename)) return false;

    strncpy(out_path, file_buffer, out_path_size - 1);
    out_path[out_path_size - 1] = '\0';
    return true;
}

bool show_open_dialog(const file_dialog_config_t* config, char* out_path, size_t out_path_size)
{
    if ((config == NULL) || (out_path == NULL) || !out_path_size) return false;

    char file_buffer[MAX_PATH] = { 0 };

    static char filter_buffer[MAX_PATH];
    int offset = 0;
    offset += snprintf(&filter_buffer[offset], sizeof(filter_buffer) - offset, "%s (*.%s)", config->filter_label, config->filter_extension);
    offset += 1;
    offset += snprintf(&filter_buffer[offset], sizeof(filter_buffer) - offset, "*.%s", config->filter_extension);
    offset += 1;
    filter_buffer[offset] = '\0';

    OPENFILENAMEA open_filename = { 0 };
    open_filename.lStructSize = sizeof(open_filename);
    open_filename.hwndOwner = (HWND)GetWindowHandle();
    open_filename.lpstrFilter = filter_buffer;
    open_filename.lpstrFile = file_buffer;
    open_filename.nMaxFile = sizeof(file_buffer);
    open_filename.lpstrDefExt = config->filter_extension;
    open_filename.lpstrTitle = config->dialog_title;
    open_filename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;   /* siehe Erklaerung unten */

    if (!GetOpenFileNameA(&open_filename)) return false;

    strncpy(out_path, file_buffer, out_path_size - 1);
    out_path[out_path_size - 1] = '\0';
    return true;
}