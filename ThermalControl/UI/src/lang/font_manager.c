/* Header */
#include "lang/font_manager.h"

/* Defines */
#define FONT_EXTRA_CHARACTERS   "ÄÖÜäöüß€°"
#define ASCII_CODEPOINT_START   32
#define ASCII_CODEPOINT_COUNT   95
#define MAX_EXTRA_CODEPOINTS    64
#define MAX_TOTAL_CODEPOINTS    (ASCII_CODEPOINT_COUNT + MAX_EXTRA_CODEPOINTS)

/* Static function definition */
static int decode_utf8_codepoints(const char* utf8_text, int* out_codepoints, int max_count)
{
    int count = 0;
    const unsigned char* current_char = (const unsigned char*)utf8_text;

    while ((*current_char != '\0') && (count < max_count))
    {
        int codepoint;
        int bytes_consumed;

        if ((*current_char & 0x80) == 0x00)
        {
            codepoint = *current_char;
            bytes_consumed = 1;
        }
        else if ((*current_char & 0xE0) == 0xC0)
        {
            codepoint = ((*current_char & 0x1F) << 6) | (current_char[1] & 0x3F);
            bytes_consumed = 2;
        }
        else if ((*current_char & 0xF0) == 0xE0)
        {
            codepoint = ((*current_char & 0x0F) << 12) | ((current_char[1] & 0x3F) << 6) | (current_char[2] & 0x3F);
            bytes_consumed = 3;
        }
        else if ((*current_char & 0xF8) == 0xF0)
        {
            codepoint = ((*current_char & 0x07) << 18) | ((current_char[1] & 0x3F) << 12) | ((current_char[2] & 0x3F) << 6) | (current_char[3] & 0x3F);
            bytes_consumed = 4;
        }
        else
        {
            current_char++;
            continue;
        }

        out_codepoints[count++] = codepoint;
        current_char += bytes_consumed;
    }

    return count;
}

static int build_codepoint_list(int* out_codepoints, int max_count)
{
    int count = 0;

    for (int codepoint_index = 0; codepoint_index < ASCII_CODEPOINT_COUNT && count < max_count; codepoint_index++)
    {
        out_codepoints[count++] = ASCII_CODEPOINT_START + codepoint_index;
    }

    count += decode_utf8_codepoints(FONT_EXTRA_CHARACTERS, out_codepoints + count, max_count - count);

    return count;
}

/* Function definition */
void load_font_family(const font_family_id_t family, const char* path, Font* font_array, const size_t array_size)
{
    if ((font_array == NULL) || !array_size) return;
    
    static int codepoints[MAX_TOTAL_CODEPOINTS];
    static int codepoint_count;
    codepoint_count = build_codepoint_list(codepoints, MAX_TOTAL_CODEPOINTS);
    
    for (font_size_t font_size_index = 0; font_size_index < FONT_SIZE_COUNT; font_size_index++)
    {
        int slot = font_id(family, font_size_index);
        int size = get_font_size_int(font_size_index);
        if (slot >= array_size) break;
        font_array[slot] = LoadFontEx(path, size, codepoints, codepoint_count);
        SetTextureFilter(font_array[slot].texture, TEXTURE_FILTER_BILINEAR);
    }
    return;
}

int font_id(const font_family_id_t family, const font_size_t size)
{
    int id = family * FONT_SIZE_COUNT + size;
    if ((id >= TOTAL_FONT_SLOTS) || (id < 0)) return 0;
    return id;
}

int get_font_size_int(const font_size_t size)
{
    int sizes[FONT_SIZE_COUNT] = { 12, 14, 16, 18, 20, 24, 32, 48 };
    if ((size < 0) || (size >= FONT_SIZE_COUNT)) return sizes[0];
    return sizes[size];
}