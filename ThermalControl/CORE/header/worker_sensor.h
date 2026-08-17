#ifndef WORKER_SENSOR_H
#define WORKER_SENSOR_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include "serial_port.h"
#include "bridge_error.h"
#include "bridge_connection.h"
#include "bridge_graph_queue.h"
#include "bridge_log_queue.h"
#include "bridge_control.h"

/* Function declarations */
void worker_sensor_start(void);
void worker_sensor_stop(void);

#endif // WORKER_SENSOR_H
