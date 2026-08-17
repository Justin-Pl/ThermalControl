#ifndef BRIDGE_GRAPH_QUEUE_H
#define BRIDGE_GRAPH_QUEUE_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>

/* Type definitions */
typedef struct
{
    uint64_t timestamp_ms;
    float temp;
    float pwm;
} graph_point_t;

/* Function declarations */
bool bridge_graph_queue_push(const graph_point_t point);   /* Producer-Thread (Core/Data) */
bool bridge_graph_queue_pull(graph_point_t* out);    /* Consumer (UI-Thread) */

#endif // BRIDGE_GRAPH_QUEUE_H