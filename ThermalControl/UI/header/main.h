#ifndef MAIN_H
#define MAIN_H

/* Source files */
#define CLAY_IMPLEMENTATION
#include "ui_constants.h"
#include "renderer/clay.h"
#include "renderer/raylib.h"
#include "renderer/clay_renderer_raylib.c"
#include "lang/lang.h"
#include "lang/font_manager.h"
#include "elements/dropdown.h"
#include "elements/topbar.h"
#include "elements/console.h"
#include "elements/temp_graph.h"
#include "elements/control_panel.h"
#include "elements/error_window.h"
#include "elements/flash_window.h"
#include "elements/warning_import.h"
#include "elements/numeric_input.h"
#include "worker_com_list.h"
#include "worker_sensor.h"
#include "worker_flash.h"
#include "serial_port.h"
#include "bridge_log_queue.h"
#include "bridge_control.h"

/* Librarys */
#include <stdio.h>
#include <stdint.h>

/* Tests */
#include "test_console.h"
#include "test_graph.h"

#endif // MAIN_H