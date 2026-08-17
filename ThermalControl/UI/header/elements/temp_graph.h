#ifndef TEMP_GRAPH_H
#define TEMP_GRAPH_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui_constants.h"
#include "renderer/clay.h"
#include "helper/custom_layout_elements.h"
#include "elements/button.h"
#include "bridge_graph_queue.h"

/* Defines */
#define GRAPH_CHUNK_SIZE                512

/* Type definitions */
typedef struct
{
    uint64_t timestamp_ms;
    double timestamp_s;
    float temp;
    float pwm;
} graph_history_point_t;

typedef struct graph_chunk_t
{
    graph_history_point_t points[GRAPH_CHUNK_SIZE];
    size_t count;
    float chunk_min_temp;
    float chunk_max_temp;
    float chunk_min_pwm;
    float chunk_max_pwm;
    double chunk_start_s;
    double chunk_end_s;

    struct graph_chunk_t* next_chunk;
} graph_chunk_t;

typedef struct
{
    graph_chunk_t* first_chunk;
    graph_chunk_t* last_chunk;
    size_t total_count;
} graph_history_t;

/* Function declarations */
void temp_graph_handle_clear_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void temp_graph_handle_auto_follow_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
void temp_graph_enable_sampling(Clay_ElementId element_id, Clay_PointerData data, void* user_data);
bool temp_graph_sampling_enabled(void);
void temp_graph_init(void);
void temp_graph_set_label_font(Font font);
void temp_graph_draw_custom(Clay_BoundingBox box, void* context);
void temp_graph_draw_for_export(Clay_BoundingBox box, float scale_factor);
bool temp_graph_get_auto_scroll_state(void);
size_t temp_graph_get_data_count(void);
void temp_graph_render(void);
bool temp_graph_export_points_to_csv(const char* filepath);
void temp_graph_clear(void);
void temp_graph_import_points(const graph_point_t* points, size_t count);

#endif // TEMP_GRAPH_H