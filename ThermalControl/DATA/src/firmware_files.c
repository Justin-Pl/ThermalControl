/* Header */
#include "firmware_files.h"

/* Libaries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Function definitions */
bool firmware_scan_files(firmware_file_list_t* out)
{
    /* Check arguments */
    if (out == NULL) return false;
    out->count = 0;

    /* Get path of executable */
    char exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if ((len == 0) || (len == sizeof(exe_path))) return false;

    /* Get the path for searching firmware files */
    char* last_slash = strrchr(exe_path, '\\');
    if (last_slash != NULL) *(last_slash + 1) = '\0';
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%sresources\\firmware\\*.hex", exe_path);

    /* Find first file, if folder is empty return empty list */
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) return true;   

    bool has_entry = true;
    while (has_entry)
    {
        /* Ignore folder */
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            /* Check if we have reached the maximum number of files */
            if (out->count >= FIRMWARE_FILES_MAX) break;

            /* Copy file name */
            firmware_file_t* current_file = &out->files[out->count];
            strncpy(current_file->name, find_data.cFileName, FIRMWARE_FILE_NAME_MAX - 1);
            current_file->name[FIRMWARE_FILE_NAME_MAX - 1] = '\0';
            out->count++;
        }

        /* Find next file */
        has_entry = FindNextFileA(find_handle, &find_data);
    }

    FindClose(find_handle);
    return true;
}