/* Header */
#include "bridge_graph_queue.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Defines */
#define GRAPH_QUEUE_CAPACITY 256

/* Type definition */
typedef struct
{
    graph_point_t buffer[GRAPH_QUEUE_CAPACITY];
    volatile LONG write_index;
    volatile LONG read_index;
} graph_queue_t;

/* Static local variables */
static graph_queue_t graph_queue = { .write_index = 0, .read_index = 0 };

/* Function definitions */
bool bridge_graph_queue_push(const graph_point_t point)
{
    LONG write_idx = graph_queue.write_index;
    LONG next_write_idx = (write_idx + 1) % GRAPH_QUEUE_CAPACITY;

    if (next_write_idx == graph_queue.read_index) return false; /* Queue voll */

    graph_queue.buffer[write_idx] = point;
    MemoryBarrier();
    graph_queue.write_index = next_write_idx;
    return true;
}

bool bridge_graph_queue_pull(graph_point_t* out)
{
    LONG read_idx = graph_queue.read_index;
    if (read_idx == graph_queue.write_index) return false; /* leer */

    *out = graph_queue.buffer[read_idx];
    MemoryBarrier();
    graph_queue.read_index = (read_idx + 1) % GRAPH_QUEUE_CAPACITY;
    return true;
}