#ifndef TOPBAR_H
#define TOPBAR_H

/* Libraries */
#include "lang/lang.h"
#include "elements/dropdown.h"
#include "elements/console.h"
#include "elements/temp_graph.h"
#include "elements/warning_import.h"
#include "elements/flash_window.h"
#include "bridge_connection.h"
#include "bridge_error.h"
#include "data_export.h"
#include "data_import.h"

/* Function declaration */
void topbar_init(void);
void topbar_render(void);

#endif // TOPBAR_H