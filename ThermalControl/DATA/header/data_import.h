#ifndef DATA_IMPORT_H
#define DATA_IMPORT_H

/* Libraries */
#include <stdio.h>
#include "lang/lang.h"
#include "elements/temp_graph.h"
#include "elements/control_panel.h"
#include "elements/console.h"
#include "file_dialog.h"
#include "bridge_connection.h"
#include "bridge_log_queue.h"
#include "bridge_error.h"
#include "bridge_control.h"

/* Function declarations */
void data_import_graph_csv(void);
void data_import_control_config(void);
void data_import_console_txt(void);

#endif // DATA_IMPORT_H