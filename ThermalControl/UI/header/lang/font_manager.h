#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

/* Libraries */
#include "renderer/clay.h"
#include "renderer/raylib.h"

/* Type definitions */
typedef enum
{
    FONT_SIZE_12 = 0,
    FONT_SIZE_14,
    FONT_SIZE_16,
    FONT_SIZE_18,
    FONT_SIZE_20,
    FONT_SIZE_24,
    FONT_SIZE_32,
    FONT_SIZE_48,
    FONT_SIZE_COUNT
} font_size_t;

typedef enum
{
    FAMILY_OPEN_SANS = 0,
    FAMILY_SEGOE,
    FONT_FAMILY_COUNT
} font_family_id_t;

/* Defines */
#define FONT_FAMILY_SIZE_COUNT		8
#define TOTAL_FONT_SLOTS            (FONT_FAMILY_COUNT * FONT_SIZE_COUNT)

/* Function declaration */
void load_font_family(const font_family_id_t family, const char* path, Font* font_array, const size_t array_size);
int font_id(const font_family_id_t family, const font_size_t size);
int get_font_size_int(const font_size_t size);

#endif // FONT_MANAGER_H