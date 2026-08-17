#ifndef WARNING_IMPORT_H
#define WARNING_IMPORT_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "lang/lang.h"
#include "elements/button.h"
#include "bridge_connection.h"
#include "bridge_log_queue.h"

/* Type definitions */
typedef void (*import_callback_t)(void);

/* Function declarations */
void warning_import_window_init(void);
void warning_import_window_show(const import_callback_t import_callback);
void warning_import_window_render(void);

#endif // WARNING_IMPORT_H