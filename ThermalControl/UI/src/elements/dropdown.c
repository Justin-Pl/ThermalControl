/* Header */
#include "elements/dropdown.h"

/* Defines */
#define DROPDOWN_MAX_DEPTH      4
#define INVALID_ID              0

/* Static local variables */
static const int dropdown_outline_width = 1;
static bool any_element_hovered = false;
static int open_path[DROPDOWN_MAX_DEPTH] = { -1 };
static uint32_t id_counter = 1;
const dropdown_sizing_t dropdown_default_sizing = 
{
    .button_width = CLAY_SIZING_FIT(0),
    .panel_width = CLAY_SIZING_FIXED(DROPDOWN_WIDTH),
    .submenu_width = CLAY_SIZING_GROW(0),
	.base_z_index = 10
};

/* Static function definition */
static void set_item_id(dropdown_item_t* item)
{
    /* Check if pointer is valid */
    if (item == NULL) return;

	item->id = id_counter++;
    if (!item->child_count) return;

    for (size_t children_index = 0; children_index < item->child_count; children_index++)
    {
		dropdown_item_t* child = (dropdown_item_t*)&item->children[children_index];
		set_item_id(child);
    }

    return;
}

static Clay_String get_item_label(const dropdown_item_t* item)
{
    if (item->use_raw_label)
    {
        return (Clay_String) { .chars = item->raw_label, .length = (int32_t)strlen(item->raw_label), .isStaticallyAllocated = false };
    }
    return get_label(item->label);
}

static void close_from_depth(int from_depth)
{
    for (int depth_index = from_depth; depth_index < DROPDOWN_MAX_DEPTH; depth_index++)
    {
        open_path[depth_index] = -1;
    }

    return;
}

static void dropdown_close_all(void)
{
    close_from_depth(0);

    return;
}


static void handle_menu_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    int menu_index = (int)(intptr_t)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        dropdown_close_all();
        if (open_path[0] != menu_index) open_path[0] = menu_index;
    }

    return;
}

static void handle_dropdown_item_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    const dropdown_item_t* item = (const dropdown_item_t*)user_data;

    if ((data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) && !item->disabled)
    {
        if (item->callback != NULL) item->callback(element_id, data, item->user_data);
        dropdown_close_all();
    }

    return;
}

static void render_items_at_depth(const dropdown_item_t* items, int item_count, int depth, Clay_SizingAxis submenu_width, int16_t base_z_index)
{
    for (int item_index = 0; item_index < item_count; item_index++)
    {
        const dropdown_item_t* item = &items[item_index];
        bool has_children = (item->children != NULL) && (item->child_count > 0);
        bool is_open = has_children && (open_path[depth] == item_index);

        Clay_Color background_color;
        Clay_Color text_color;
        Clay_ElementId button_id = CLAY_SIDI(CLAY_STRING("DropdownMenuItem"), item->id);
        bool is_hovered = Clay_PointerOver(button_id);

        if (item->disabled)
        {
            background_color = get_color(APP_COLOR_BUTTON_BACKGROUND_DISABLED);
            text_color = get_color(APP_COLOR_BUTTON_TEXT_DISABLED);
        }
        else
        {
            background_color = (is_hovered || is_open)
                ? get_color(APP_COLOR_ITEM_HOVER_BACKGROUND)
                : get_color(APP_COLOR_PANEL_BACKGROUND);
            text_color = get_color(APP_COLOR_TEXT);
        }

        CLAY(CLAY_IDI("DropdownMenuItem", item->id),
        {
            .layout =
            {
                .sizing = { .width = submenu_width },
                .padding = CLAY_PADDING_ALL(MIN_PADDING),
                .childGap = HALF_PADDING,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
        })
        {
            if (Clay_Hovered())
            {
                any_element_hovered = true;

                if (has_children)
                {
                    if (open_path[depth] != item_index)
                    {
                        open_path[depth] = item_index;
                        close_from_depth(depth + 1);
                    }
                }
                else
                {
                    if (open_path[depth] != -1)
                    {
                        close_from_depth(depth);
                    }
                }
            }

            CLAY_TEXT(get_item_label(item),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = text_color
            });

            if (has_children)
            {
                CLAY(CLAY_IDI("DropdownItemSpacer", item->id), 
                {
                    .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
                }) {}

                CLAY_TEXT(CLAY_STRING(">"),
                {
                    .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                    .fontSize = get_font_size_int(BASE_FONT_SIZE),
                    .textColor = text_color
                });
            }
            else
            {
                Clay_OnHover(handle_dropdown_item_interaction, (void*)item);
            }

            if (is_open)
            {
                CLAY(CLAY_IDI("DropdownSubmenuPanel", item->id),
                {
                    .layout =
                    {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = { .width = CLAY_SIZING_FIXED(DROPDOWN_WIDTH) },
                        .padding = CLAY_PADDING_ALL(MIN_PADDING),
                        .childGap = MIN_PADDING
                    },
                    .backgroundColor = get_color(APP_COLOR_PANEL_BACKGROUND),
                    .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS),
                    .border = 
                    {
                        .color = get_color(APP_COLOR_PANEL_OUTLINE),
                        .width = CLAY_BORDER_OUTSIDE(dropdown_outline_width)
                    },
                    .floating =
                    {
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .attachPoints =
                        {
                            .parent = CLAY_ATTACH_POINT_RIGHT_TOP,
                            .element = CLAY_ATTACH_POINT_LEFT_TOP
                        },
                        .zIndex = base_z_index + depth
                    }
                })
                {
                    if (Clay_Hovered()) any_element_hovered = true;

                    render_items_at_depth((void*)item->children, item->child_count, depth + 1, submenu_width, base_z_index);
                }
            }
        }
    }
}

/* Function definition */
void dropdown_init(void)
{
    memset(open_path, -1, sizeof(open_path));

    return;
}

void dropdown_register_menu(dropdown_item_t* items, const size_t item_count)
{
    if ((items == NULL) || !item_count) return;
	
    for (size_t item_index = 0; item_index < item_count; item_index++)
    {
        set_item_id(&items[item_index]);
    }

    return;
}

void dropdown_render_menu(Clay_String label, const dropdown_item_t* items, int item_count, bool disabled, const dropdown_sizing_t* sizing)
{
    if (items == NULL) return;

    const dropdown_sizing_t* current_size = sizing != NULL ? sizing : &dropdown_default_sizing;

    bool effectively_disabled = disabled || (item_count == 0);

    uint32_t menu_index = items[0].id;
    bool is_open = !effectively_disabled && (open_path[0] == menu_index);

    if (effectively_disabled && (open_path[0] == menu_index)) dropdown_close_all();

    Clay_ElementId button_id = CLAY_SIDI(CLAY_STRING("DropdownMenuButton"), menu_index);
    bool is_hovered = Clay_PointerOver(button_id);

    Clay_Color background_color;
    Clay_Color text_color;

    if (effectively_disabled)
    {
        background_color = get_color(APP_COLOR_BUTTON_BACKGROUND_DISABLED);
        text_color = get_color(APP_COLOR_BUTTON_TEXT_DISABLED);
    }
    else
    {
        background_color = (is_open || is_hovered) ? get_color(APP_COLOR_ITEM_HOVER_BACKGROUND) : get_color(APP_COLOR_ITEM_BACKGROUND);
        text_color = get_color(APP_COLOR_TEXT);
    }

    CLAY(CLAY_IDI("DropdownMenuButton", menu_index),
    {
        .layout = 
        { 
            .padding = CLAY_PADDING_ALL(HALF_PADDING),
            .sizing = { .width = current_size->button_width }
        },
        .backgroundColor = background_color,
        .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS)
    })
    {
        if (!effectively_disabled)
        {
            if (Clay_Hovered()) any_element_hovered = true;
            Clay_OnHover(handle_menu_button_interaction, (void*)(intptr_t)menu_index);
        }

        CLAY_TEXT(label, 
        { 
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE), 
            .fontSize = get_font_size_int(BASE_FONT_SIZE), 
            .textColor = text_color
        });

        if (is_open)
        {
            CLAY(CLAY_IDI("DropdownMenuPanel", menu_index),
            {
                .layout = 
                {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = { .width = current_size->panel_width },
                    .padding = CLAY_PADDING_ALL(MIN_PADDING),
                    .childGap = MIN_PADDING
                },
                .backgroundColor = get_color(APP_COLOR_PANEL_BACKGROUND),
                .cornerRadius = CLAY_CORNER_RADIUS(STANDARD_RADIUS),
                .floating = 
                {
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                    .attachPoints = 
                    {
                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                        .element = CLAY_ATTACH_POINT_LEFT_TOP
                    },
                    .zIndex = current_size->base_z_index
                }
            })
            {
                if (Clay_Hovered()) any_element_hovered = true;

                render_items_at_depth((void*)items, item_count, 1, current_size->submenu_width, current_size->base_z_index);
            }
        }
    }
}

void dropdown_handle_click_outside(void)
{
    if (open_path[0] == -1)
    {
        any_element_hovered = false;
        return;
    }

    Clay_PointerData pointer = Clay_GetPointerState();

    if (pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME && !any_element_hovered)
    {
        dropdown_close_all();
    }

    any_element_hovered = false;
}