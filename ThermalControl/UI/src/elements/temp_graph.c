/* Header */
#include "elements\temp_graph.h"

/* Defines */
#define GRAPH_PWM_MAX                   100.0f
#define GRAPH_PWM_MIN                   0.0f
#define GRAPH_TEMP_MAX                  200.0f
#define GRAPH_TEMP_MIN                  -100.0f
#define GRAPH_Y_AXIS_LEFT_MARGIN        55    
#define GRAPH_Y_AXIS_RIGHT_MARGIN       50  
#define GRAPH_BOTTOM_MARGIN             10        
#define GRAPH_X_LABEL_HEIGHT            20        
#define GRAPH_CLAY_ID                   "TempGraph"
#define GRAPH_FONT_SIZE                 14
#define GRAPH_FONT_SPACING              1
#define GRAPH_LINE_TARGET_NUM           6
#define GRAPH_LABEL_HORIZONTAL_SPACING  4
#define GRAPH_LINE_THICKNESS            3.0f
#define GRAPH_SEPERATOR_LINE_WIDTH      2
#define GRAPH_BAR_HEIGHT                50
#define GRAPH_TARGET_POINT_COUNT        64
#define GRAPH_LIVE_TAIL_POINT_COUNT     16
#define GRAPH_MAX_STRIDE                4

/* Type definitions */
typedef struct
{
    double visible_start_s;
    double visible_end_s;
    bool initialized;
} graph_view_t;

typedef struct
{
    float min_temp;
    float max_temp;
    float min_pwm;
    float max_pwm;
} graph_axis_range_t;

/* Static local variables */
static graph_history_t graph_data;
static graph_view_t current_view = { .visible_start_s = 0, .visible_end_s = 0, .initialized = false };
static Font graph_label_font;
static uint64_t graph_first_point_tick_ms = 0;
static bool graph_anchor_set = false;
static CustomLayoutElement graph_custom_element = { .type = CUSTOM_LAYOUT_ELEMENT_TYPE_GRAPH };
static bool graph_auto_follow_s = true;
static button_config_t clear_button;
static button_config_t auto_follow_button;
static bool enable_sampling = false;

/* Static function definition */
static bool graph_point_valid(const graph_point_t point)
{
    if (!isfinite(point.temp) || !isfinite(point.pwm)) return false;
    return true;
}

static bool graph_history_append(graph_history_t* history, graph_point_t point_ms)
{
    /* Check if history is valid */
    if (history == NULL) return false;

    /* Check if point is valid */
    if (!graph_point_valid(point_ms)) return false;

    /* Set first time anchor if not set */
    if (!graph_anchor_set)
    {
        graph_first_point_tick_ms = point_ms.timestamp_ms;
        graph_anchor_set = true;
    }

    /* If last chunk is empty or last chunk is full append new chunk */
    if ((history->last_chunk == NULL) || (history->last_chunk->count >= GRAPH_CHUNK_SIZE))
    {
        /* Allocate memory for new chunk */
        graph_chunk_t* new_chunk = (graph_chunk_t*)malloc(sizeof(graph_chunk_t));
        if (new_chunk == NULL) return false;

        /* Reset point count & next chunk */
        new_chunk->count = 0;
        new_chunk->next_chunk = NULL;
        new_chunk->chunk_min_temp = FLT_MAX;
        new_chunk->chunk_max_temp = -FLT_MAX;
        new_chunk->chunk_min_pwm = FLT_MAX;
        new_chunk->chunk_max_pwm = -FLT_MAX;

        /* If last chunk is empty (first point), set new chunk to first & last chunk */
        if (history->last_chunk == NULL)
        {
            history->first_chunk = new_chunk;
        }
        else /* After first point append all chunks as next chunk */
        {
            history->last_chunk->next_chunk = new_chunk;
        }

        history->last_chunk = new_chunk;
    }

    /* Get extreme values for chunk */
    graph_chunk_t* chunk = history->last_chunk;
    graph_history_point_t point_s =
    {
        .timestamp_ms = point_ms.timestamp_ms,
        .timestamp_s = (double)(point_ms.timestamp_ms - graph_first_point_tick_ms) / 1000.0,
        .temp = point_ms.temp,
        .pwm = point_ms.pwm
    };
    if (point_s.temp < chunk->chunk_min_temp) chunk->chunk_min_temp = point_s.temp;
    if (point_s.temp > chunk->chunk_max_temp) chunk->chunk_max_temp = point_s.temp;
    if (point_s.pwm < chunk->chunk_min_pwm) chunk->chunk_min_pwm = point_s.pwm;
    if (point_s.pwm > chunk->chunk_max_pwm) chunk->chunk_max_pwm = point_s.pwm;
    if (chunk->count == 0) chunk->chunk_start_s = point_s.timestamp_s;
    chunk->chunk_end_s = point_s.timestamp_s;

    /* Append point to chunk */
    chunk->points[chunk->count++] = point_s;
    history->total_count++;
    return true;
}

static void graph_drain_queue(void)
{
    /* Pull all points from queue & append it to the graph */
    graph_point_t point;
    while (bridge_graph_queue_pull(&point))
    {
        graph_history_append(&graph_data, point);
    }
    return;
}

static void graph_reset_history(graph_history_t* history)
{
    /* Check if history is valid */
    if (history == NULL) return;

    /* Free all chunks */
    graph_chunk_t* current_chunk = history->first_chunk;
    while (current_chunk != NULL)
    {
        graph_chunk_t* next_chunk = current_chunk->next_chunk;
        free(current_chunk);
        current_chunk = next_chunk;
    }

    /* Reset pointers & count */
    history->first_chunk = NULL;
    history->last_chunk = NULL;
    history->total_count = 0;

    /* Reset time anchor */
    graph_anchor_set = false;

    return;
}

static void graph_handle_zoom(Clay_BoundingBox plot_area)
{
    /* Get graph id & if pointer is not over it return */
    Clay_ElementId graph_id = Clay_GetElementId(CLAY_STRING(GRAPH_CLAY_ID));
    if (!Clay_PointerOver(graph_id)) return;

    /* If wheel is not moved skip */
    float wheel_y = GetMouseWheelMoveV().y;
    if (wheel_y == 0) return;

    /* If wheel is spinned deactivate auto-follow */
    graph_auto_follow_s = false;

    /* Get current mouse position */
    Clay_Vector2 mouse_pos = Clay_GetPointerState().position;

    /* Get mouse position in the graph (0-1) */
    float mouse_fraction = (mouse_pos.x - plot_area.x) / plot_area.width;
    if (mouse_fraction < 0) mouse_fraction = 0;
    if (mouse_fraction > 1) mouse_fraction = 1;

    /* Get current visible range */
    double visible_range_s = current_view.visible_end_s - current_view.visible_start_s;
    double mouse_time_s = current_view.visible_start_s + (double)mouse_fraction * visible_range_s;

    /* Scroll up = zoom in, Scroll down = zoom out */
    float zoom_factor = (wheel_y > 0) ? 0.9f : (1.0f / 0.9f);

    /* Get new range for zoom */
    double new_range_s = visible_range_s * (double)zoom_factor;

    /* Cap to min range */
    const double MIN_RANGE_S = 10.0;
    if (new_range_s < MIN_RANGE_S) new_range_s = MIN_RANGE_S;

    /* Get new total range */
    double total_data_range_s = (graph_data.last_chunk != NULL)
        ? (graph_data.last_chunk->chunk_end_s - graph_data.first_chunk->chunk_start_s)
        : new_range_s;
    if (new_range_s > total_data_range_s) new_range_s = total_data_range_s;

    /* Activate auto_follow if fully zoomed out */
    if ((total_data_range_s > 0.0) && (new_range_s >= (0.99 * total_data_range_s)))
    {
        graph_auto_follow_s = true;
    }
    else
    {
        graph_auto_follow_s = false;
    }

    double left_amount_s = (double)mouse_fraction * new_range_s;
    double right_amount_s = (1.0 - (double)mouse_fraction) * new_range_s;

    double earliest_s = (graph_data.first_chunk != NULL) ? graph_data.first_chunk->chunk_start_s : 0.0;
    double candidate_start_s = mouse_time_s - left_amount_s;

    current_view.visible_start_s = (candidate_start_s < earliest_s) ? earliest_s : candidate_start_s;
    current_view.visible_end_s = mouse_time_s + right_amount_s;

    return;
}

static graph_axis_range_t graph_compute_visible_range(double start_s, double end_s)
{
    graph_axis_range_t range = { .min_temp = FLT_MAX, .max_temp = -FLT_MAX, .min_pwm = FLT_MAX, .max_pwm = -FLT_MAX };
    bool found_any = false;

    /* Get range from all data points */
    graph_chunk_t* chunk = graph_data.first_chunk;
    while (chunk != NULL)
    {
        /* If chunk is empty skip it */
        if (chunk->count == 0) 
        { 
            chunk = chunk->next_chunk; 
            continue; 
        }

        /* If chunk is out of bounds skip it */
        if ((chunk->chunk_end_s < start_s) || (chunk->chunk_start_s > end_s))
        {
            chunk = chunk->next_chunk;
            continue;
        }

        /* Chunk is completly in visible range, so check chunk values */
        if ((chunk->chunk_start_s >= start_s) && (chunk->chunk_end_s <= end_s))
        {
            found_any = true;
            if (chunk->chunk_min_temp < range.min_temp) range.min_temp = chunk->chunk_min_temp;
            if (chunk->chunk_max_temp > range.max_temp) range.max_temp = chunk->chunk_max_temp;
            if (chunk->chunk_min_pwm < range.min_pwm) range.min_pwm = chunk->chunk_min_pwm;
            if (chunk->chunk_max_pwm > range.max_pwm) range.max_pwm = chunk->chunk_max_pwm;
        }
        else
        {
            /* Chunk is overlapping with view, so check all points */
            for (size_t point_index = 0; point_index < chunk->count; point_index++)
            {
                graph_history_point_t* current_point = &chunk->points[point_index];
                
                /* If point is out of range skip it */
                if ((current_point->timestamp_s < start_s) || (current_point->timestamp_s > end_s)) continue;
                
                /* Point found, check it */
                found_any = true;
                if (current_point->temp < range.min_temp) range.min_temp = current_point->temp;
                if (current_point->temp > range.max_temp) range.max_temp = current_point->temp;
                if (current_point->pwm < range.min_pwm) range.min_pwm = current_point->pwm;
                if (current_point->pwm > range.max_pwm) range.max_pwm = current_point->pwm;
            }
        }
        chunk = chunk->next_chunk;
    }

    /* If not found any points in range use default range */
    if (!found_any)
    {
        range.min_temp = 0.0f; range.max_temp = 1.0f;
        range.min_pwm = 0.0f; range.max_pwm = 100.0f;
        return range;
    }

    /* Apply padding to temperature range */
    float temp_span = range.max_temp - range.min_temp;
    float temp_padding = temp_span * 0.1f;
    if (temp_padding < 0.5f) temp_padding = 0.5f;
    range.min_temp -= temp_padding;
    range.max_temp += temp_padding;
    range.min_pwm = 0.0f;
    range.max_pwm = 100.0f;
    return range;

    /* Apply padding to pwm range */
    float pwm_span = range.max_pwm - range.min_pwm;
    float pwm_padding = pwm_span * 0.1f;
    if (pwm_padding < 1.0f) pwm_padding = 1.0f;
    range.min_pwm -= pwm_padding;
    range.max_pwm += pwm_padding;

    return range;
}

static double graph_compute_nice_step(double range, int target_lines)
{
    /* Check inputs */
    if ((range <= 0) || (target_lines <= 0)) return 1.0;

    /* Compute a rough step size */
    double rough_step = range / (double)target_lines;
    double magnitude = pow(10.0, floor(log10(rough_step)));
    double residual = rough_step / magnitude;

    /* Compute a "nice" step size */
    double nice_residual;
    if (residual < 1.5)
    {
        nice_residual = 1.0;
    }
    else if (residual < 3.0)
    {
        nice_residual = 2.0;
    }
    else if (residual < 7.0)
    {
        nice_residual = 5.0;
    }
    else
    {
        nice_residual = 10.0;
    }

    return nice_residual * magnitude;
}

static void temp_graph_draw_internal(Clay_BoundingBox box, bool skip_zoom, float scale_factor)
{
    char label_buf[32];

    /* Get colors for all elements */
    Color grid_color = get_color_raylib(APP_COLOR_GRAPH_GRID);
    Color temp_color = get_color_raylib(APP_COLOR_GRAPH_TEMP);
    Color pwm_color = get_color_raylib(APP_COLOR_GRAPH_PWM);
    Color time_color = get_color_raylib(APP_COLOR_TEXT);

    float scaled_font_size = GRAPH_FONT_SIZE * scale_factor;
    float scaled_line_thickness = GRAPH_LINE_THICKNESS * scale_factor;
    float scaled_left_margin = GRAPH_Y_AXIS_LEFT_MARGIN * scale_factor;
    float scaled_right_margin = GRAPH_Y_AXIS_RIGHT_MARGIN * scale_factor;
    float scaled_bottom_margin = GRAPH_BOTTOM_MARGIN * scale_factor;
    float scaled_x_label_height = GRAPH_X_LABEL_HEIGHT * scale_factor;
    float scaled_top_margin = (scaled_font_size / 2.0f) + (GRAPH_LABEL_HORIZONTAL_SPACING * scale_factor);
    float scaled_label_spacing = GRAPH_LABEL_HORIZONTAL_SPACING * scale_factor;

    /* Plot box with padding to the edges */
    Clay_BoundingBox plot_box = box;
    plot_box.x = box.x + scaled_left_margin;
    plot_box.y = box.y + scaled_top_margin;                                                   
    plot_box.width = box.width - scaled_left_margin - scaled_right_margin;
    plot_box.height = box.height - scaled_top_margin - scaled_bottom_margin - scaled_x_label_height;

    /* Ensure minimum size */
    if (plot_box.width < 10.0f) plot_box.width = 10.0f;
    if (plot_box.height < 10.0f) plot_box.height = 10.0f;

    /* Handle zooming */
    if (!skip_zoom) graph_handle_zoom(plot_box);

    /* Auto-follow mode */
    if (graph_auto_follow_s && (graph_data.last_chunk != NULL))
    {
        /* Get time range */
        double earliest_s = graph_data.first_chunk->chunk_start_s;
        double latest_s = graph_data.last_chunk->chunk_end_s;

        /* Apply padding to the visible time range */
        double time_padding_s = (latest_s - earliest_s) * 0.05;
        if (time_padding_s < 1.0) time_padding_s = 1.0;

        /* Set time view range */
        current_view.visible_start_s = earliest_s;
        current_view.visible_end_s = latest_s + time_padding_s;
    }

    /* Initialize view */
    if (!current_view.initialized)
    {
        /* Set view to current timespan or if no data is available set to default values */
        if (graph_data.first_chunk != NULL)
        {
            current_view.visible_start_s = graph_data.first_chunk->chunk_start_s;
            current_view.visible_end_s = graph_data.last_chunk->chunk_end_s;
        }
        else
        {
            current_view.visible_start_s = 0.0;
            current_view.visible_end_s = 60.0;
        }
        current_view.initialized = true;
    }

    /* Compute visible range */
    graph_axis_range_t axis = graph_compute_visible_range(current_view.visible_start_s, current_view.visible_end_s);

    /* Get the visible time range & ensure the result */
    double visible_range_s = current_view.visible_end_s - current_view.visible_start_s;
    if (visible_range_s <= 0.0) visible_range_s = 1.0;

    /* Get the temp & pwm range & ensure the result */
    double temp_range = (double)(axis.max_temp - axis.min_temp);
    if (temp_range <= 0.0) temp_range = 1.0;
    double pwm_range = (double)(axis.max_pwm - axis.min_pwm);
    if (pwm_range <= 0.0) pwm_range = 1.0;

    /* Vertical lines for time */
    double time_step_s = graph_compute_nice_step(visible_range_s, GRAPH_LINE_TARGET_NUM);
    double first_time_gridline_s = floor(current_view.visible_start_s / time_step_s) * time_step_s;
    for (double time_step = first_time_gridline_s; time_step <= current_view.visible_end_s; time_step += time_step_s)
    {
        if (time_step < current_view.visible_start_s) continue;

        /* Calculate x coordinate */
        float x_fraction = (float)((time_step - current_view.visible_start_s) / visible_range_s);
        float x = plot_box.x + x_fraction * plot_box.width;

        /* Draw vertical line */
        DrawLine((int)x, (int)plot_box.y, (int)x, (int)(plot_box.y + plot_box.height), (Color)grid_color);

        /* Draw label text */
        snprintf(label_buf, sizeof(label_buf), "%.0fs", time_step);
        Vector2 text_size = MeasureTextEx(graph_label_font, label_buf, scaled_font_size, GRAPH_FONT_SPACING);
        Vector2 text_pos = { .x = x - text_size.x / 2.0f, .y = plot_box.y + plot_box.height + scaled_bottom_margin };
        DrawTextEx(graph_label_font, label_buf, text_pos, scaled_font_size, GRAPH_FONT_SPACING, (Color)time_color);
    }

    /* Horizontal lines for temp & pwm */
    double temp_step = graph_compute_nice_step(temp_range, GRAPH_LINE_TARGET_NUM);
    double first_temp_gridline = floor(axis.min_temp / temp_step) * temp_step;
    for (double temp_value = first_temp_gridline; temp_value <= axis.max_temp; temp_value += temp_step)
    {
        /* If temp value is out of bounds skip */
        if (temp_value < axis.min_temp) continue;

        /* Calculate y coordinate */
        float value_fraction = (float)((temp_value - axis.min_temp) / temp_range);
        float y = plot_box.y + plot_box.height * (1.0f - value_fraction);

        /* Draw horizontal line */
        DrawLine((int)plot_box.x, (int)y, (int)(plot_box.x + plot_box.width), (int)y, (Color)grid_color);

        /* Temp label on the left side */
        snprintf(label_buf, sizeof(label_buf), "%.1f", temp_value);
        Vector2 temp_text_size = MeasureTextEx(graph_label_font, label_buf, scaled_font_size, GRAPH_FONT_SPACING);
        Vector2 temp_text_pos = { .x = plot_box.x - temp_text_size.x - scaled_label_spacing, .y = y - temp_text_size.y / 2.0f };
        DrawTextEx(graph_label_font, label_buf, temp_text_pos, scaled_font_size, GRAPH_FONT_SPACING, (Color)temp_color);

        /* PWM label on the right side */
        double pwm_value_here = axis.min_pwm + (double)value_fraction * pwm_range;
        snprintf(label_buf, sizeof(label_buf), "%.0f%%", pwm_value_here);
        Vector2 pwm_text_size = MeasureTextEx(graph_label_font, label_buf, scaled_font_size, GRAPH_FONT_SPACING);
        Vector2 pwm_text_pos = { .x = plot_box.x + plot_box.width + scaled_label_spacing, .y = y - pwm_text_size.y / 2.0f };
        DrawTextEx(graph_label_font, label_buf, pwm_text_pos, scaled_font_size, GRAPH_FONT_SPACING, (Color)pwm_color);
    }

    /* Count visible points via chunks */
    long visible_point_count = 0;
    graph_chunk_t* count_chunk = graph_data.first_chunk;
    while (count_chunk != NULL)
    {
        /* If chunk is not in view, skip it */
        if ((count_chunk->chunk_end_s < current_view.visible_start_s) || (count_chunk->chunk_start_s > current_view.visible_end_s))
        {
            count_chunk = count_chunk->next_chunk;
            continue;
        }

        /* If chunk is completly in view add whole count */
        if ((count_chunk->chunk_start_s >= current_view.visible_start_s) && (count_chunk->chunk_end_s <= current_view.visible_end_s))
        {
            visible_point_count += (long)count_chunk->count;
        }
        else
        {
            /* Iterate through chunks, which overlap with view */
            for (size_t point_index = 0; point_index < count_chunk->count; point_index++)
            {
                if ((count_chunk->points[point_index].timestamp_s >= current_view.visible_start_s) &&
                    (count_chunk->points[point_index].timestamp_s <= current_view.visible_end_s))
                {
                    visible_point_count++;
                }
            }
        }
        count_chunk = count_chunk->next_chunk;
    }

    /* Calculate sampling factor */
    int sampling_factor = 1;
    while ((visible_point_count / sampling_factor) > GRAPH_TARGET_POINT_COUNT && (sampling_factor < GRAPH_MAX_STRIDE))
    {
        sampling_factor *= 2;
    }
    if (!enable_sampling) sampling_factor = 1;

    /* Get the beginning of the tail, the tail is used for time acurate new data at the end of the curve */
    double tail_start_s = DBL_MAX; 
    if (graph_data.last_chunk != NULL)
    {
        graph_chunk_t* last = graph_data.last_chunk;
        size_t tail_count = (last->count < GRAPH_LIVE_TAIL_POINT_COUNT) ? last->count : GRAPH_LIVE_TAIL_POINT_COUNT;
        if (tail_count > 0)
        {
            size_t tail_start_index = last->count - tail_count;
            tail_start_s = last->points[tail_start_index].timestamp_s;
        }
    }

    graph_chunk_t* chunk = graph_data.first_chunk;
    Vector2 last_temp_point = { 0 };
    Vector2 last_pwm_point = { 0 };
    bool has_last_point = false;

    int points_in_group = 0;
    float group_sum_temp = 0.0f;
    float group_sum_pwm = 0.0f;
    double group_start_s = 0.0;

    uint32_t draw_calls = 0;
    long points_processed_in_view = 0;
    bool reached_tail = false;

    /* Check if tail is visible */
    double actual_data_end_s = (graph_data.last_chunk != NULL) ? graph_data.last_chunk->chunk_end_s : 0.0;
    bool viewing_live_edge = (current_view.visible_end_s >= actual_data_end_s);

    long historical_point_limit;
    if (viewing_live_edge) // Tail visible in view
    {
        historical_point_limit = visible_point_count - GRAPH_LIVE_TAIL_POINT_COUNT;
        if (historical_point_limit < 0) historical_point_limit = 0;
        historical_point_limit = (historical_point_limit / sampling_factor) * sampling_factor;
    }
    else
    {
        /* Tail not visible in view */
        historical_point_limit = visible_point_count;
    }

    /* Iterate through all chunks until tail is reached */
    while ((chunk != NULL) && !reached_tail)
    {
        /* Chunk is out of visible view, skip it */
        if ((chunk->chunk_end_s < current_view.visible_start_s) || (chunk->chunk_start_s > current_view.visible_end_s))
        {
            has_last_point = false;
            chunk = chunk->next_chunk;
            continue;
        }

        /* Iterate through all points in the chunk */
        for (size_t point_index = 0; point_index < chunk->count; point_index++)
        {
            graph_history_point_t* data_point = &chunk->points[point_index];

            /* If the point is out of visible view, skip it */
            if ((data_point->timestamp_s < current_view.visible_start_s) || (data_point->timestamp_s > current_view.visible_end_s))
            {
                has_last_point = false;
                continue;
            }

            /* If tail is reached skip all forward points */
            if (points_processed_in_view >= historical_point_limit)
            {
                reached_tail = true;
                break;
            }

            points_processed_in_view++;

            /* Draw every point in x1 sampling */
            if (sampling_factor == 1)
            {
                float x_fraction = (float)((data_point->timestamp_s - current_view.visible_start_s) / visible_range_s);
                float x = plot_box.x + x_fraction * plot_box.width;
                float temp_y = plot_box.y + plot_box.height * (1.0f - (data_point->temp - axis.min_temp) / temp_range);
                float pwm_y = plot_box.y + plot_box.height * (1.0f - (data_point->pwm - axis.min_pwm) / pwm_range);

                Vector2 temp_point = { x, temp_y };
                Vector2 pwm_point = { x, pwm_y };

                if (has_last_point)
                {
                    DrawLineEx(last_temp_point, temp_point, scaled_line_thickness, (Color)temp_color);
                    DrawLineEx(last_pwm_point, pwm_point, scaled_line_thickness, (Color)pwm_color);
                    draw_calls += 2;
                }
                last_temp_point = temp_point;
                last_pwm_point = pwm_point;
                has_last_point = true;
            }
            else
            {
                /* Add point to group */
                if (points_in_group == 0) group_start_s = data_point->timestamp_s;
                group_sum_temp += data_point->temp;
                group_sum_pwm += data_point->pwm;
                points_in_group++;

                /* If sampling rate is matched, calculate average & draw point */
                if (points_in_group >= sampling_factor)
                {
                    double group_mid_s = (group_start_s + data_point->timestamp_s) / 2.0;
                    float x_fraction = (float)((group_mid_s - current_view.visible_start_s) / visible_range_s);
                    float x = plot_box.x + x_fraction * plot_box.width;

                    float temp_avg_y = plot_box.y + plot_box.height * (1.0f - (group_sum_temp / points_in_group - axis.min_temp) / temp_range);
                    float pwm_avg_y = plot_box.y + plot_box.height * (1.0f - (group_sum_pwm / points_in_group - axis.min_pwm) / pwm_range);

                    Vector2 temp_point = { x, temp_avg_y };
                    Vector2 pwm_point = { x, pwm_avg_y };

                    if (has_last_point)
                    {
                        DrawLineEx(last_temp_point, temp_point, scaled_line_thickness, (Color)temp_color);
                        DrawLineEx(last_pwm_point, pwm_point, scaled_line_thickness, (Color)pwm_color);
                        draw_calls += 2;
                    }
                    last_temp_point = temp_point;
                    last_pwm_point = pwm_point;
                    has_last_point = true;

                    points_in_group = 0;
                    group_sum_temp = 0.0f;
                    group_sum_pwm = 0.0f;
                }
            }
        }

        /* If tail is not reached, go to next chunk */
        if (!reached_tail) chunk = chunk->next_chunk;
    }

    /* Draw the tail with x1 sampling  */
    if (viewing_live_edge && (graph_data.last_chunk != NULL))
    {
        graph_chunk_t* last = graph_data.last_chunk;
        size_t tail_count = (last->count < GRAPH_LIVE_TAIL_POINT_COUNT) ? last->count : GRAPH_LIVE_TAIL_POINT_COUNT;
        size_t tail_start_index = last->count - tail_count;

        for (size_t point_index = tail_start_index; point_index < last->count; point_index++)
        {
            graph_history_point_t* data_point = &last->points[point_index];
            if (data_point->timestamp_s < current_view.visible_start_s || data_point->timestamp_s > current_view.visible_end_s) continue;

            float x_fraction = (float)((data_point->timestamp_s - current_view.visible_start_s) / visible_range_s);
            float x = plot_box.x + x_fraction * plot_box.width;
            float temp_y = plot_box.y + plot_box.height * (1.0f - (data_point->temp - axis.min_temp) / temp_range);
            float pwm_y = plot_box.y + plot_box.height * (1.0f - (data_point->pwm - axis.min_pwm) / pwm_range);

            Vector2 temp_point = { x, temp_y };
            Vector2 pwm_point = { x, pwm_y };

            if (has_last_point)
            {
                DrawLineEx(last_temp_point, temp_point, scaled_line_thickness, (Color)temp_color);
                DrawLineEx(last_pwm_point, pwm_point, scaled_line_thickness, (Color)pwm_color);
                draw_calls += 2;
            }
            last_temp_point = temp_point;
            last_pwm_point = pwm_point;
            has_last_point = true;
        }
    }    

    // printf("DrawLineEx: %u (sampling_factor=%d, sichtbare Punkte=%ld)\n\r", draw_calls, sampling_factor, visible_point_count);

    return;
}

static void temp_graph_render_data_counter(void)
{
    static char data_point_counter_text[32];
    static char pad_text[16];
    size_t total_count = graph_data.total_count;

    /* Calculate current padding length */
    int current_digits = snprintf(data_point_counter_text, sizeof(data_point_counter_text), "%u", total_count);
    int max_digits = snprintf(NULL, 0, "%u", (unsigned)SIZE_MAX);
    int pad_count = max_digits - current_digits;
    if (pad_count < 0) pad_count = 0;
    if (pad_count > 15) pad_count = 7;

    /* Fill padding text with zeros */
    memset(pad_text, '0', pad_count);
    pad_text[pad_count] = '\0';

    /* Build full text for data point counter */
    static char full_text[32];
    const lang_string_t* description = lang_get(STRING_ID_DATA_POINTS);
    snprintf(full_text, sizeof(full_text), "%llu %s", total_count, description->chars);

    /* Clay countainer without a child gap */
    CLAY(CLAY_ID("DataPointCounterWrapper"),
    {
        .layout = {.childGap = 0 }
    })
    {
        /* Invisible padding */
        Clay_String pad_string = { .chars = pad_text, .length = (int32_t)strlen(pad_text), .isStaticallyAllocated = false };
        CLAY_TEXT(pad_string,
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_BOX_BACKGROUND)
        });

        /* Visible data point counter */
        Clay_String counter_string = { .chars = full_text, .length = (int32_t)strlen(full_text), .isStaticallyAllocated = false };
        CLAY_TEXT(counter_string,
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
    }

    return;
}

static void temp_graph_render_control_bar(void)
{
    CLAY(CLAY_ID("TempGraphBar"),
    {
        .layout =
        {
            .sizing = { CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(GRAPH_BAR_HEIGHT) },
            .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = MIN_PADDING,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
        .cornerRadius = {.topLeft = STANDARD_RADIUS, .topRight = STANDARD_RADIUS },
        .border =
        {
            .color = get_color(APP_COLOR_PANEL_OUTLINE),
            .width = {.bottom = GRAPH_SEPERATOR_LINE_WIDTH }
        }
    }) 
    {
        /* Clear-Button */
        clear_button.enabled = graph_data.total_count ? true : false;
        button_render(&clear_button);

        /* Auto scroll button */
        auto_follow_button.enabled = graph_auto_follow_s ? false : true;
        button_render(&auto_follow_button); 

        /* Seperator */
        CLAY_TEXT(CLAY_STRING("|"),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });

        /* Spacer to shift to the right */
        CLAY(CLAY_ID("GraphBarSpacer"),
        {
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0) } }
        }) {}

        /* Data point counter */
        temp_graph_render_data_counter();
    }

    return;
}

/* Function definition */
void temp_graph_handle_clear_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        graph_reset_history(&graph_data);
    }
    return;
}

void temp_graph_handle_auto_follow_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        graph_auto_follow_s = true;
    }
    return;
}

void temp_graph_enable_sampling(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        enable_sampling = !enable_sampling;
    }
    return;
}

bool temp_graph_sampling_enabled(void)
{
    return enable_sampling;
}

void temp_graph_init(void)
{
    /* Initialize button */
    memset(&clear_button, 0, sizeof(clear_button));
    button_init(&clear_button);
    clear_button.labels.enabled_label = STRING_ID_CLEAR;
    clear_button.labels.disabled_label = STRING_ID_CLEAR;
    clear_button.enabled = false;
    clear_button.is_toggle_button = false;
    clear_button.callback = temp_graph_handle_clear_button_interaction;

    memset(&auto_follow_button, 0, sizeof(auto_follow_button));
    button_init(&auto_follow_button);
    auto_follow_button.labels.enabled_label = STRING_ID_AUTO_FOLLOW;
    auto_follow_button.labels.disabled_label = STRING_ID_AUTO_FOLLOW;
    auto_follow_button.enabled = false;
    auto_follow_button.is_toggle_button = false;
    auto_follow_button.callback = temp_graph_handle_auto_follow_button_interaction;
    return;
}

void temp_graph_set_label_font(Font font)
{
    graph_label_font = font;
    return;
}

void temp_graph_draw_custom(Clay_BoundingBox box, void* context)
{
    (void)context;
    temp_graph_draw_internal(box, false, 1.0f);   
    return;
}

void temp_graph_draw_for_export(Clay_BoundingBox box, float scale_factor)
{
    temp_graph_draw_internal(box, true, scale_factor);   
    return;
}

bool temp_graph_get_auto_scroll_state(void)
{
    return graph_auto_follow_s;
}

size_t temp_graph_get_data_count(void)
{
    return graph_data.total_count;
}

void temp_graph_render(void)
{
    graph_drain_queue();

    CLAY(CLAY_ID("TempGraphWrapper"),
    {
        .layout =
        {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = get_color(APP_COLOR_BACKGROUND),
    })
    {
        temp_graph_render_control_bar();

        CLAY(CLAY_ID("GraphWrapper"),
        {
            .layout = 
            {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = { .top = STANDARD_PADDING, .bottom = STANDARD_PADDING }
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = {.bottomLeft = STANDARD_RADIUS, .bottomRight = STANDARD_RADIUS },
        })
        {
            CLAY(CLAY_ID(GRAPH_CLAY_ID),
            {
                .layout = {.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
                .custom = {.customData = &graph_custom_element }
            }) {}
        }
    }

    return;
}

bool temp_graph_export_points_to_csv(const char* filepath)
{
    if (filepath == NULL) return false;

    FILE* file = fopen(filepath, "w");
    if (file == NULL) return false;

    fprintf(file, "# ClayTest Messdaten-Export\n");
    fprintf(file, "# Zeitstempel_UTC_ms,Zeit_seit_Beginn_s,Temperatur_C,PWM_Prozent\n");

    graph_chunk_t* chunk = graph_data.first_chunk;
    while (chunk != NULL)
    {
        for (size_t point_index = 0; point_index < chunk->count; point_index++)
        {
            graph_history_point_t* point = &chunk->points[point_index];

            fprintf(file, "%llu,%.3f,%.2f,%.2f\n",
                point->timestamp_ms,
                point->timestamp_s,
                point->temp,
                point->pwm);
        }
        chunk = chunk->next_chunk;
    }

    fclose(file);
    return true;
}

void temp_graph_clear(void)
{
    /* Clear old history */
    graph_reset_history(&graph_data);
    current_view.initialized = false;
    return;
}

void temp_graph_import_points(const graph_point_t* points, size_t count)
{
    /* Check if arguments are valid */
    if ((points == NULL) || !count) return;

    /* Append imported points */
    for (size_t point_index = 0; point_index < count; point_index++)
    {
        graph_history_append(&graph_data, points[point_index]);
    }

    return;
}