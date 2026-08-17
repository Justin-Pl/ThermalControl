#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

/* Libraries */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* Type definitions */
typedef struct
{
    const char* dialog_title;      
    const char* filter_label;      
    const char* filter_extension;  
    const char* default_filename;
} file_dialog_config_t;

/* Function declarations */
bool show_save_dialog(const file_dialog_config_t* config, char* out_path, size_t out_path_size);
bool show_open_dialog(const file_dialog_config_t* config, char* out_path, size_t out_path_size);

#endif // FILE_DIALOG_H