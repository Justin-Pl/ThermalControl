#ifndef FLASH_WINDOW_H
#define FLASH_WINDOW_H

/* Libraries */
#include "ui_constants.h"
#include "renderer/clay.h"
#include "lang/lang.h"
#include "lang/font_manager.h"
#include "elements/button.h"
#include "elements/dropdown.h"
#include "firmware_files.h"
#include "bridge_flash.h"

/* Function declarations */
void flash_window_init(void);
void flash_window_show(void);
void flash_window_render(void);

#endif // FLASH_WINDOW_H