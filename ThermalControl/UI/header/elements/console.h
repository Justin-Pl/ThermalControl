#ifndef CONSOLE_H
#define CONSOLE_H

/* Libraries */
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "ui_constants.h"
#include "bridge_connection.h"
#include "renderer/clay.h"
#include "lang/lang.h"
#include "lang/font_manager.h"
#include "elements/button.h"
#include "bridge_log_queue.h"

/* Type definitions */
typedef bool (*console_pull_function_t)(console_message_t* message);

/* Function declaration */
void console_handle_clear_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void console_handle_auto_follow_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void console_init(void);
void console_register_pull_function(console_pull_function_t func);
void console_set_auto_scroll(bool enabled);
bool console_get_auto_scroll_state(void);
uint32_t console_get_line_counter(void);
void console_render(void);
bool console_export_to_txt(const char* filepath);
void console_import_raw_line(const char* raw_line);
void console_clear(void);

#endif // CONSOLE_H