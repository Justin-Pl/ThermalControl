#ifndef FIRMWARE_FILES_H
#define FIRMWARE_FILES_H

/* Libraries */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Defines */
#define FIRMWARE_FILES_MAX		16
#define FIRMWARE_FILE_NAME_MAX	64

/* Type definitions */
typedef struct 
{
    char name[FIRMWARE_FILE_NAME_MAX];
} firmware_file_t;

typedef struct
{
	firmware_file_t files[FIRMWARE_FILES_MAX];
	size_t count;
} firmware_file_list_t;

/* Function declarations */
bool firmware_scan_files(firmware_file_list_t* out);

#endif // FIRMWARE_FILES_H