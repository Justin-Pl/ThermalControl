#ifndef WORKER_FLASH_H
#define WORKER_FLASH_H

/* Libraries */
#include "serial_port.h"
#include "bridge_connection.h"
#include "bridge_flash.h"
#include "bridge_log_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Function declarations */
void worker_flash_start(void);
void worker_flash_stop(void);

#endif // WORKER_FLASH_H