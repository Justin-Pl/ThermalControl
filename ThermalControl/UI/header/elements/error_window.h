#ifndef ERROR_WINDOW_H
#define ERROR_WINDOW_H

/* Libraries */
#include <stdint.h>
#include "ui_constants.h"
#include "bridge_error.h"
#include "lang/lang.h"
#include "renderer/clay.h"

/* Function declarations */
void error_window_init(void);
void error_window_render(void);

#endif // ERROR_WINDOW_H