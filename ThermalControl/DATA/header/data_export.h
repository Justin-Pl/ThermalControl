#ifndef DATA_EXPORT_H
#define DATA_EXPORT_H

/* Libraries */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "ui_constants.h"
#include "renderer/raylib.h"
#include "elements/temp_graph.h"
#include "elements/console.h"
#include "file_dialog.h"
#include "bridge_log_queue.h"
#include "bridge_error.h"
#include "bridge_control.h"

/* Function declarations */
void data_export_graph_csv(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void data_export_graph_png(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void data_export_console_txt(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void data_export_control_config_cfg(Clay_ElementId element_id, Clay_PointerData data, void* user_data);

#endif // DATA_EXPORT_H