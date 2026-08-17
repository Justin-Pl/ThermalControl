#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

/* Libraries */
#include <stdio.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "lang/lang.h"
#include "elements/dropdown.h"
#include "elements/button.h"
#include "elements/checkbox.h"
#include "elements/numeric_input.h"
#include "bridge_connection.h"
#include "bridge_control.h"
#include "bridge_log_queue.h"

/* Function declaration */
void control_panel_init(void);
void control_panel_apply_imported_config(const bridge_control_config_t* config);
void control_panel_render(void);

#endif // CONTROL_PANEL_H
