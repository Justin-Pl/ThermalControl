/* Header */
#include "elements\console.h"

/* Defines */
#define CONSOLE_MAX_SOURCES			4
#define CONSOLE_MAX_LINES			512
#define SCROLLBAR_WIDTH				8
#define SCROLLBAR_MIN_THUMB_HEIGHT	20
#define CONNECT_LOGO_SIZE			18

/* Static type definitions */
typedef struct
{
	console_pull_function_t pull;
	console_message_t staged;
	bool has_staged;
} console_source_t;

typedef struct
{
	char line[CONSOLE_MAX_MESSAGE_LENGTH];
	uint64_t uid;
} console_line_t;

/* Static local variables */
static const int console_outline_width = 2;
static console_source_t sources[CONSOLE_MAX_SOURCES];
static size_t source_count = 0;
static console_line_t lines[CONSOLE_MAX_LINES];
static uint32_t write_index = 0;
static uint32_t line_count = 0;
static uint64_t line_uid = 0;
static bool auto_scroll_enabled = true;
static bool scrollbar_dragging = false;
static float scrollbar_drag_start_mouse_y = 0;
static float scrollbar_drag_start_scroll_y = 0;
static bool has_last_known_scroll_y = false;
static float last_known_scroll_y = 0;
static button_config_t clear_button;
static button_config_t auto_follow_button;
static uint64_t first_log_tick_ms = 0;
static bool log_time_anchor_set = false;

/* Images */
static Texture2D connect_logo;
static Texture2D disconnect_logo;

/* Static function definition */
static void format_timestamp_prefix(uint64_t msg_tick_ms, char* out, size_t out_size)
{
	/* Check parameters */
	if (!out_size || (out == NULL)) return;

	/* Calculate seconds and milliseconds */
	time_t seconds = (time_t)(msg_tick_ms / 1000);
	uint32_t millis = (uint32_t)(msg_tick_ms % 1000);

	/* Convert to local time */
	struct tm local_time;
	localtime_s(&local_time, &seconds);

	/* Format string */
	snprintf(out, out_size, "[%02d:%02d:%02d-%03u]  ", local_time.tm_hour, local_time.tm_min, local_time.tm_sec, millis);

	return;
}

static void console_add_line(const char* text, uint64_t timestamp_ms)
{
	if (text == NULL) return;

	char prefix[32];
	format_timestamp_prefix(timestamp_ms, prefix, sizeof(prefix));

	console_line_t* current_line = &lines[write_index % CONSOLE_MAX_LINES];
	snprintf(current_line->line, CONSOLE_MAX_MESSAGE_LENGTH, "%s%s", prefix, text);
	current_line->line[CONSOLE_MAX_MESSAGE_LENGTH - 1] = '\0';
	current_line->uid = line_uid++;

	write_index = (write_index + 1) % CONSOLE_MAX_LINES;
	if (line_count < CONSOLE_MAX_LINES) line_count++;

	return;
}

static void console_drain_sources(void)
{
	/* Early return if no sources are registered */
	if (!source_count) return;

	/* Iterate through all sources and pull first message*/
	for (size_t source_index = 0; source_index < source_count; source_index++)
	{
		/* Get current source */
		console_source_t* current_source = &sources[source_index];

		/* Check if message was already pulled */
		if (current_source->has_staged) continue;

		/* Pull message & change state if successfull */
		bool pulled = current_source->pull(&current_source->staged);
		current_source->has_staged = pulled;
	}

	/* Drain until no message is left */
	while (true)
	{
		int16_t earliest_index = -1;
		uint64_t earliest_timestamp = 0;

		/* Search for the earlist message from all queues */
		for (size_t source_index = 0; source_index < source_count; source_index++)
		{
			/* Get current source */
			console_source_t* current_source = &sources[source_index];
		
			/* If source is empty skip */
			if (!current_source->has_staged) continue;
		
			/* If earlist index is valid & time is bigger than earlist timestamp skip */
			if ((earliest_index >= 0) && (current_source->staged.timestamp_ms >= earliest_timestamp)) continue;
		
			/* New earlist time */
			earliest_index = source_index;
			earliest_timestamp = current_source->staged.timestamp_ms;
		}

		/* If all sources are empty exit */
		if (earliest_index < 0) break;

		/* Set log time anchor if not set */
		if (!log_time_anchor_set)
		{
			first_log_tick_ms = sources[earliest_index].staged.timestamp_ms;
			log_time_anchor_set = true;
		}

		/* Log */
		console_add_line(sources[earliest_index].staged.plain_text, sources[earliest_index].staged.timestamp_ms);

		/* Pull next message from current source to compare */
		bool pulled = sources[earliest_index].pull(&sources[earliest_index].staged);
		sources[earliest_index].has_staged = pulled;
	}

	return;
}

static void console_render_lines(void)
{
	/* Early return if no lines are present */
	if (!line_count) return;

	/* Render all lines */
	uint32_t start_index = (line_count < CONSOLE_MAX_LINES) ? 0 : write_index;
	for (size_t line_index = 0; line_index < line_count; line_index++)
	{
		uint32_t current_index = (start_index + line_index) % CONSOLE_MAX_LINES;
		console_line_t* current_line = &lines[current_index];
		CLAY(CLAY_IDI("ConsoleLine", current_line->uid),
		{
			.layout =
			{
				.sizing = {.width = CLAY_SIZING_GROW(0) }
			}
		})
		{
			Clay_String text_string = { .length = (int32_t)strlen(current_line->line), .chars = current_line->line };
			CLAY_TEXT((text_string),
			{
				.fontId = font_id(FAMILY_OPEN_SANS, FONT_SIZE_18),
				.fontSize = get_font_size_int(FONT_SIZE_18),
				.textColor = get_color(APP_COLOR_TEXT)
			});
		}
	}
	return;
}

static void console_render_scrollbar(void)
{
	/* Get parent & scroll data */
	Clay_ElementId console_id = Clay_GetElementId(CLAY_STRING("Console"));
	Clay_ScrollContainerData scroll_data = Clay_GetScrollContainerData(console_id);

	/* Check if scroll data is valid */
	if (!scroll_data.found || (scroll_data.scrollPosition == NULL)) return;

	/* Get dimensions */
	float content_height = scroll_data.contentDimensions.height;
	float box_height = scroll_data.scrollContainerDimensions.height;

	/* If no scrolling is needed, return */
	if (content_height <= box_height) return;

	/* Calculate maximum scroll position */
	float max_scroll = content_height - box_height;

	/* Calculate thumb height, fix to minimum if necessary */
	float thumb_height = (box_height / content_height) * box_height;
	if (thumb_height < SCROLLBAR_MIN_THUMB_HEIGHT) thumb_height = SCROLLBAR_MIN_THUMB_HEIGHT;
	if (thumb_height > box_height) thumb_height = box_height;

	/* Calculate scroll fraction */
	float scroll_fraction = (max_scroll > 0) ? (-scroll_data.scrollPosition->y / max_scroll) : 0;
	if (scroll_fraction < 0) scroll_fraction = 0;
	if (scroll_fraction > 1) scroll_fraction = 1;

	/* Calculate thumb offset */
	float thumb_travel = box_height - thumb_height;
	float thumb_offset_y = scroll_fraction * thumb_travel;

	/* Clay layout */
	CLAY(CLAY_ID("ConsoleScrollbarTrack"),
	{
		.layout = 
		{
			.sizing = { .width = CLAY_SIZING_FIXED(SCROLLBAR_WIDTH), .height = CLAY_SIZING_FIXED(box_height) }
		},
		.floating =
		{
			.attachTo = CLAY_ATTACH_TO_PARENT,
			.attachPoints = { .parent = CLAY_ATTACH_POINT_RIGHT_TOP, .element = CLAY_ATTACH_POINT_RIGHT_TOP },
			.zIndex = 20
		}
	})
	{
		CLAY(CLAY_ID("ConsoleScrollbarThumb"),
		{
			.layout = 
			{
				.sizing = {.width = CLAY_SIZING_FIXED(SCROLLBAR_WIDTH), .height = CLAY_SIZING_FIXED(thumb_height) } 
			},
			.backgroundColor = Clay_Hovered() ? get_color(APP_COLOR_ITEM_HOVER_BACKGROUND) : get_color(APP_COLOR_PANEL_OUTLINE),
			.cornerRadius = CLAY_CORNER_RADIUS(SCROLLBAR_WIDTH / 2),
			.floating =
			{
				.attachTo = CLAY_ATTACH_TO_PARENT,
				.attachPoints = {.parent = CLAY_ATTACH_POINT_RIGHT_TOP, .element = CLAY_ATTACH_POINT_RIGHT_TOP },
				.offset = {.x = 0, .y = thumb_offset_y },
				.zIndex = 21
			}
		})
		{
		}
	}

	/* Drag Handling, click on thumb activates dragging */
	Clay_PointerData pointer = Clay_GetPointerState();
	Clay_ElementId thumb_id = Clay_GetElementId(CLAY_STRING("ConsoleScrollbarThumb"));

	/* Activate dragging when thumb is clicked */
	if ((pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) && Clay_PointerOver(thumb_id))
	{
		scrollbar_dragging = true;
		scrollbar_drag_start_mouse_y = pointer.position.y;
		scrollbar_drag_start_scroll_y = scroll_data.scrollPosition->y;
		auto_scroll_enabled = false;
	}

	/* Handle dragging */
	if (scrollbar_dragging)
	{
		/* Handle drag movement */
		if ((pointer.state == CLAY_POINTER_DATA_PRESSED) || (pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME))
		{
			/* Calculate scroll delta based on mouse movement */
			float mouse_delta_y = pointer.position.y - scrollbar_drag_start_mouse_y;
			float scroll_delta = (thumb_travel > 0) ? (mouse_delta_y / thumb_travel) * max_scroll : 0;

			/* Get new scroll position */
			float new_scroll_y = scrollbar_drag_start_scroll_y - scroll_delta;
			if (new_scroll_y > 0) new_scroll_y = 0;
			if (new_scroll_y < -max_scroll) new_scroll_y = -max_scroll;

			scroll_data.scrollPosition->y = new_scroll_y;
		}
		else
		{
			scrollbar_dragging = false;
		}
	}

	return;
}

static void console_render_connection_info(void)
{
	/* Get current connection info, thread safe */
	connection_info_t connect_info;
	bridge_get_connection_info(&connect_info);

	/* Connection info for UI */
	Texture2D* current_logo;
	string_id_t current_connect_string;
	static char com_port_name[16];
	static char com_baud_rate[16];

	/* Fill UI elements dependig on connection */
	if (connect_info.connected)
	{
		current_logo = &connect_logo;
		current_connect_string = STRING_ID_CONNECTED;
		if ((connect_info.port_number < COM_PORT_NUM_MIN) || (connect_info.port_number > COM_PORT_NUM_MAX))
		{
			strcpy(com_port_name, "COM ???");
		}
		else
		{
			sprintf(com_port_name, "COM %d", connect_info.port_number);
		}
		if ((connect_info.baud_rate < COM_PORT_BAUD_MIN) || (connect_info.baud_rate > COM_PORT_BAUD_MAX))
		{
			strcpy(com_baud_rate, "BAUD ???");
		}
		else
		{
			sprintf(com_baud_rate, "BAUD %d", connect_info.baud_rate);
		}
	}
	else
	{
		current_logo = &disconnect_logo;
		current_connect_string = STRING_ID_DISCONNECTED;
		strcpy(com_port_name, "COM ???");
		strcpy(com_baud_rate, "BAUD ???");
	}

	/* Connection logo & status */
	CLAY(CLAY_ID("ConnectLogo"),
	{
		.layout = 
		{ 
			.sizing = {.width = CLAY_SIZING_FIXED(CONNECT_LOGO_SIZE), .height = CLAY_SIZING_FIXED(CONNECT_LOGO_SIZE) }, 
			.padding = {.top = 0, .bottom = 0, .left = MIN_PADDING, .right = 0 }
		},
		.image = { .imageData = current_logo }
	}) {}
	CLAY_TEXT(get_label(current_connect_string),
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	/* Seperator */
	CLAY_TEXT(CLAY_STRING("|"),
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	/* COM port name */
	Clay_String com_string = { .chars = com_port_name, .length = strlen(com_port_name) };
	CLAY_TEXT(com_string,
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	/* Seperator */
	CLAY_TEXT(CLAY_STRING("|"),
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	/* COM port baud rate */
	Clay_String com_baud_string = { .chars = com_baud_rate, .length = strlen(com_baud_rate) };
	CLAY_TEXT(com_baud_string,
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	return;
}

static void console_render_line_counter(void)
{
	/* Text for line counter & invisible padding text */
	static char line_counter_text[16];
	static char pad_text[8];

	/* Calculate current padding length */
	int current_digits = snprintf(line_counter_text, sizeof(line_counter_text), "%u", line_count);
	int max_digits = snprintf(NULL, 0, "%u", (unsigned)CONSOLE_MAX_LINES);
	int pad_count = max_digits - current_digits;
	if (pad_count < 0) pad_count = 0;
	if (pad_count > 7) pad_count = 7;

	/* Fill padding text with zeros */
	memset(pad_text, '0', pad_count);
	pad_text[pad_count] = '\0';

	/* Build full text for visible line counter */
	static char full_text[32];
	snprintf(full_text, sizeof(full_text), "%u / %u (max.)", line_count, (unsigned)CONSOLE_MAX_LINES);

	/* Clay countainer without a child gap */
	CLAY(CLAY_ID("LineCounterWrapper"), 
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

		/* Visible line counter */
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

static void console_render_bar_controls(void)
{
	/* Spacer to shift to the right */
	CLAY(CLAY_ID("ConsoleBarSpacer"),
	{
		.layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
	}) {}

	/* Clear-Button */
	clear_button.enabled = line_count ? true : false;
	button_render(&clear_button);

	/* Auto scroll button */
	auto_follow_button.enabled = auto_scroll_enabled ? false : true;
	button_render(&auto_follow_button);

	/* Seperator */
	CLAY_TEXT(CLAY_STRING("|"),
	{
		.fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
		.fontSize = get_font_size_int(BASE_FONT_SIZE),
		.textColor = get_color(APP_COLOR_TEXT)
	});

	/* Line counter */
	console_render_line_counter();

	return;
}

/* Function definition */
void console_handle_clear_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
	(void)element_id;
	(void)user_data;

	if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
	{
		console_clear();
	}
	return;
}

void console_handle_auto_follow_button_interaction(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
	(void)element_id;
	(void)user_data;

	if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
	{
		auto_scroll_enabled = true;
	}
	return;
}

void console_init(void)
{
	/* Load images */
	connect_logo = LoadTexture("resources/images/connect.png");
	disconnect_logo = LoadTexture("resources/images/disconnect.png");

	/* Initialize button */
	memset(&clear_button, 0, sizeof(clear_button));
	button_init(&clear_button);
	clear_button.labels.enabled_label = STRING_ID_CLEAR;
	clear_button.labels.disabled_label = STRING_ID_CLEAR;
	clear_button.enabled = false;
	clear_button.is_toggle_button = false;
	clear_button.callback = console_handle_clear_button_interaction;

	memset(&auto_follow_button, 0, sizeof(auto_follow_button));
	button_init(&auto_follow_button);
	auto_follow_button.labels.enabled_label = STRING_ID_AUTO_FOLLOW;
	auto_follow_button.labels.disabled_label = STRING_ID_AUTO_FOLLOW;
	auto_follow_button.enabled = false;
	auto_follow_button.is_toggle_button = false;
	auto_follow_button.callback = console_handle_auto_follow_button_interaction;

	return;
}

void console_register_pull_function(console_pull_function_t func)
{
	if (source_count >= CONSOLE_MAX_SOURCES) return;
	if (func == NULL) return;
	sources[source_count].pull = func;
	sources[source_count].has_staged = false;
	source_count++;
	return;
}

void console_set_auto_scroll(bool enabled)
{
	auto_scroll_enabled = enabled;
	return;
}

bool console_get_auto_scroll_state(void)
{
	return auto_scroll_enabled;
}

uint32_t console_get_line_counter(void)
{
	return line_count;
}

void console_render(void)
{
	CLAY(CLAY_ID("ConsoleWrapper"),
	{
		.layout =
		{
			.sizing = { .width = CLAY_SIZING_GROW(0) },
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
		},
		.backgroundColor = get_color(APP_COLOR_BACKGROUND),
	})
	{ 
		CLAY(CLAY_ID("Console"),
		{
			.layout =
			{
				.sizing = { CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(CONSOLE_HEIGHT) },
				.padding = CLAY_PADDING_ALL(STANDARD_PADDING),
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.childGap = MIN_PADDING
			},
			.backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
			.cornerRadius = { .topLeft = STANDARD_RADIUS, .topRight = STANDARD_RADIUS, .bottomLeft = 0, .bottomRight = 0 },
			.clip = {.vertical = true, .childOffset = Clay_GetScrollOffset() }
		})
		{
			console_drain_sources();
			console_render_lines();
		}

		console_render_scrollbar();

		CLAY(CLAY_ID("ConsoleBar"),
		{
			.layout =
			{
				.sizing = { CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(CONSOLE_BAR_HEIGHT) },
				.padding = CLAY_PADDING_ALL(STANDARD_PADDING),
				.layoutDirection = CLAY_LEFT_TO_RIGHT,
				.childGap = MIN_PADDING,
				.childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
			},
			.backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
			.cornerRadius = { .topLeft = 0, .topRight = 0, .bottomLeft = STANDARD_RADIUS, .bottomRight = STANDARD_RADIUS },
			.border =
			{
				.color = get_color(APP_COLOR_PANEL_OUTLINE),
				.width = {.top = console_outline_width, .left = 0, .right = 0, .bottom = 0, .betweenChildren = 0 }
			}
		})
		{
			console_render_connection_info();
			console_render_bar_controls();
		}
	}

	/* Handle auto scroll */
	Clay_ScrollContainerData scroll_data = Clay_GetScrollContainerData(Clay_GetElementId(CLAY_STRING("Console")));
	if (scroll_data.found && (scroll_data.scrollPosition != NULL))
	{
		/* Get if scroll has changed */
		float max_scroll = scroll_data.contentDimensions.height - scroll_data.scrollContainerDimensions.height;

		/* Scroll was detected */
		if (max_scroll > 0)
		{
			/* Scroll was detected without auto-scroll */
			if (has_last_known_scroll_y && !scrollbar_dragging && (scroll_data.scrollPosition->y != last_known_scroll_y))
			{
				auto_scroll_enabled = false;
			}

			/* If auto-scroll is enabled, jump to the bottom */
			if (auto_scroll_enabled)
			{
				scroll_data.scrollPosition->y = -max_scroll;
			}

			/* If scroll is near the bottom, enable auto-scroll */
			float scroll_fraction = -scroll_data.scrollPosition->y / max_scroll;
			if ((scroll_fraction >= 0.99f) && !scrollbar_dragging)
			{
				auto_scroll_enabled = true;
			}

			/* Update last known scroll position */
			last_known_scroll_y = scroll_data.scrollPosition->y;
			has_last_known_scroll_y = true;
		}
	}

	return;
}

bool console_export_to_txt(const char* filepath)
{
	if (filepath == NULL) return false;

	FILE* file = fopen(filepath, "w");
	if (file == NULL) return false;

	uint32_t start_index = (line_count < CONSOLE_MAX_LINES) ? 0 : write_index;
	for (uint32_t line_index = 0; line_index < line_count; line_index++)
	{
		uint32_t current_index = (start_index + line_index) % CONSOLE_MAX_LINES;
		fprintf(file, "%s\n", lines[current_index].line);
	}

	fclose(file);
	return true;
}

void console_import_raw_line(const char* raw_line)
{
	if (raw_line == NULL) return;

	console_line_t* current_line = &lines[write_index % CONSOLE_MAX_LINES];
	snprintf(current_line->line, CONSOLE_MAX_MESSAGE_LENGTH, "%s", raw_line);
	current_line->line[CONSOLE_MAX_MESSAGE_LENGTH - 1] = '\0';
	current_line->uid = line_uid++;

	write_index = (write_index + 1) % CONSOLE_MAX_LINES;
	if (line_count < CONSOLE_MAX_LINES) line_count++;

	return;
}
void console_clear(void)
{
	line_count = 0;
	write_index = 0;
	line_uid = 0;

	return;
}