#ifndef DROPDOWN_H
#define DROPDOWN_H

/* Libraries */
#include <stdint.h>
#include <string.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "lang/lang.h"
#include "lang/font_manager.h"

/* Defines */
#define DROPDOWN_MAX_ITEM_NAME_LENGTH       32
#define DROPDOWN_WIDTH                      200

/* Type definitions */
typedef void (*dropdown_item_callback_t)(Clay_ElementId element_id, Clay_PointerData data, void* user_data);

typedef struct
{
    Clay_SizingAxis button_width;
    Clay_SizingAxis panel_width;
    Clay_SizingAxis submenu_width;
    int16_t base_z_index;
} dropdown_sizing_t;

typedef struct dropdown_item_t
{
    string_id_t label;
    char raw_label[DROPDOWN_MAX_ITEM_NAME_LENGTH];
    bool use_raw_label;
    uint32_t id;
    bool disabled;
    dropdown_item_callback_t callback;
    void* user_data;
    struct dropdown_item_t* children;
    size_t child_count;
} dropdown_item_t;

/* Function declaration */
void dropdown_init(void);
void dropdown_register_menu(dropdown_item_t* items, const size_t item_count);
void dropdown_render_menu(Clay_String label, const dropdown_item_t* items, int item_count, bool disabled, const dropdown_sizing_t* sizing);
void dropdown_handle_click_outside(void);

#endif // DROPDOWN_H