/* Header */
#include "elements/control_panel.h"

/* Defines */
#define CONTROL_PANEL_WIDTH                     300
#define CONTROL_PANEL_LINE_WIDTH                2
#define CONTROL_PANEL_MANUAL_NUMERIC_PWM_MAX    100.0f
#define CONTROL_PANEL_MANUAL_NUMERIC_PWM_MIN    0.0f
#define CONTROL_PANEL_SET_POINT_MAX             150.0f
#define CONTROL_PANEL_SET_POINT_MIN             0.0f
#define CONTROL_PANEL_HYSTERESE_MAX             10.0f
#define CONTROL_PANEL_HYSTERESE_MIN             0.0f
#define CONTROL_PANEL_PID_MAX                   999.99f
#define CONTROL_PANEL_PID_MIN                   0.0f
#define CONTROL_PANEL_VARIABLE_LEFT_PADDING     32

/* Static local variables */
static com_port_list_t current_list;
static connection_info_t com_info;
static dropdown_item_t com_ports[COM_PORT_NUM_MAX - 1];
static button_config_t com_reset_button;
static button_config_t com_connect_button;
static button_config_t com_auto_connect_button;
static const int32_t possible_baud_rates[] =
{
    9600,
    14400,
    19200,
    28800,
    38400,
    57600,
    115200
};
static dropdown_item_t baud_rates[sizeof(possible_baud_rates) / sizeof(possible_baud_rates[0])];
static const dropdown_sizing_t menu_sizing_grow =
{
    .button_width = CLAY_SIZING_GROW(0),
    .panel_width = CLAY_SIZING_GROW(0),
    .submenu_width = CLAY_SIZING_GROW(0)
};
static Clay_Color connect_idle_color;
static Clay_Color connect_active_color;
static checkbox_config_t enable_manual_control;
static checkbox_config_t enable_two_point_control;
static checkbox_config_t enable_auto_control;
static checkbox_config_t enable_p_term;
static checkbox_config_t enable_i_term;
static checkbox_config_t enable_d_term;
static numeric_input_config_t manual_pwm_numeric;
static numeric_input_config_t set_point_two_point_numeric;
static numeric_input_config_t hysterese_two_point_numeric;
static numeric_input_config_t set_point_pid_numeric;
static numeric_input_config_t kp_numeric;
static numeric_input_config_t ki_numeric;
static numeric_input_config_t kd_numeric;
static bridge_control_config_t last_control_config;

/* Static function definitions */
static bool control_panel_com_info_valid(void)
{
    if ((com_info.port_number > COM_PORT_NUM_MAX) || (com_info.port_number < COM_PORT_NUM_MIN)) return false;
    if ((com_info.baud_rate > COM_PORT_BAUD_MAX) || (com_info.baud_rate < COM_PORT_BAUD_MIN)) return false;
    return true;
}

static void control_panel_select_port(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    uint16_t com_num = (uint16_t)(intptr_t)user_data;
    com_info.port_number = com_num;
    return;
}

static void control_panel_render_com_port_list(void)
{
    if (bridge_get_com_ports_if_changed(&current_list))
    {
        for (size_t port_index = 0; port_index < current_list.com_count; port_index++)
        {
            bool com_valid = true;
            uint16_t com_num = current_list.com_numbers[port_index];
            if ((com_num < COM_PORT_NUM_MIN) || (com_num > COM_PORT_NUM_MAX)) com_valid = false;

            dropdown_item_t* item = &com_ports[port_index];
            if (com_valid)
            {
                snprintf(item->raw_label, sizeof(item->raw_label), "COM %d", com_num);
                item->disabled = false;
                item->callback = control_panel_select_port;
                item->user_data = (void*)(intptr_t)com_num;
            }
            else
            {
                strncpy(item->raw_label, "COM ???", sizeof(item->raw_label));
                item->disabled = true;
                item->user_data = NULL;
            }
            item->use_raw_label = true;

        }
    }
    
    static char current_selected_port[32];
    memset(current_selected_port, 0, sizeof(current_selected_port));
    connection_info_t current_connection;
    bridge_get_connection_info(&current_connection);
    if (current_connection.connected)
    {
        snprintf(current_selected_port, sizeof(current_selected_port), "COM %d", current_connection.port_number);
        com_info.port_number = current_connection.port_number;
    }
    else
    {
        if ((com_info.port_number >= COM_PORT_NUM_MIN) && (com_info.port_number <= COM_PORT_NUM_MAX))
        {
            snprintf(current_selected_port, sizeof(current_selected_port), "COM %d", com_info.port_number);
        }
        else
        {
            strncpy(current_selected_port, "COM ???", sizeof(current_selected_port));
        }
    }
    static Clay_String current_port_label;
    current_port_label.chars = current_selected_port;
    current_port_label.length = strlen(current_selected_port);
    bool menu_disabled = current_list.com_count ? false : true;
    if (com_connect_button.toggled_state) menu_disabled = true;

    CLAY(CLAY_ID("COM_LIST_WRAPPER"),
    {
        .layout =
        {
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .childGap = STANDARD_PADDING
        },
    }) 
    {
        CLAY(CLAY_ID("COM_LIST_LABEL"),
        {
            .layout =
            {
                .sizing = { .width = (CONTROL_PANEL_WIDTH / 2) - (STANDARD_PADDING * 2)}
            },
        })
        {
            CLAY_TEXT(get_label(STRING_ID_COM),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });
        }
        dropdown_render_menu(current_port_label, com_ports, current_list.com_count, menu_disabled, &menu_sizing_grow);
    }

    return;
}

static void control_panel_render_baud_list(void)
{
    static char current_selected_baud[32];
    memset(current_selected_baud, 0, sizeof(current_selected_baud));
    if ((com_info.baud_rate >= COM_PORT_BAUD_MIN) && (com_info.baud_rate <= COM_PORT_BAUD_MAX))
    {
        snprintf(current_selected_baud, sizeof(current_selected_baud), "%d", com_info.baud_rate);
    }
    else
    {
        strncpy(current_selected_baud, "???", sizeof(current_selected_baud));
    }
    static Clay_String current_baud_label;
    current_baud_label.chars = current_selected_baud;
    current_baud_label.length = strlen(current_selected_baud);

    CLAY(CLAY_ID("BAUD_LIST_WRAPPER"),
    {
        .layout =
        {
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .childGap = STANDARD_PADDING
        },
    })
    {
        CLAY(CLAY_ID("BAUD_LIST_LABEL"),
        {
            .layout =
            {
                .sizing = { .width = (CONTROL_PANEL_WIDTH / 2) - (STANDARD_PADDING * 2)}
            },
        })
        {
            CLAY_TEXT(get_label(STRING_ID_BAUD),
            {
                .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
                .fontSize = get_font_size_int(BASE_FONT_SIZE),
                .textColor = get_color(APP_COLOR_TEXT)
            });
        }
        bool disable_menu = false;
        if (com_connect_button.toggled_state) disable_menu = true;
        dropdown_render_menu(current_baud_label, baud_rates, sizeof(baud_rates) / sizeof(baud_rates[0]), disable_menu, &menu_sizing_grow);
    }

    return;
}

static void control_panel_render_com_buttons(void)
{
    connect_idle_color = get_color(APP_COLOR_TOGGLE_ON);
    connect_active_color = get_color(APP_COLOR_TOGGLE_OFF);
    connection_info_t con_info;
    if (bridge_get_connection_info(&con_info))
    {
        com_connect_button.toggled_state = con_info.connected;
    }

    CLAY(CLAY_ID("COM_BUTTON_WRAPPER"),
    {
        .layout =
        {
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap = MIN_PADDING
        },
    })
    {
        bool baud_valid = ((com_info.baud_rate <= COM_PORT_BAUD_MAX) && (com_info.baud_rate >= COM_PORT_BAUD_MIN));
        com_auto_connect_button.enabled = baud_valid && !con_info.connected && !con_info.auto_connect_running ? true : false;
        button_render(&com_auto_connect_button);

        CLAY(CLAY_ID("COM_CONNECTION_WRAPPER"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            },
        })
        {
            com_connect_button.enabled = control_panel_com_info_valid() && !con_info.auto_connect_running ? true : false;
            com_connect_button.labels.disabled_label = com_connect_button.toggled_state ? STRING_ID_DISCONNECT : STRING_ID_CONNECT;
            button_render(&com_connect_button);

            CLAY(CLAY_ID("ComButtonSpacer"),
                {
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0) } }
                }) {
            }

            bool com_info_is_empty = (com_info.port_number == 0) && (com_info.baud_rate == 0);
            com_reset_button.enabled = com_info_is_empty || con_info.auto_connect_running ? false : true;
            if (com_connect_button.toggled_state) com_reset_button.enabled = false;
            button_render(&com_reset_button);
        }
    }
    return;
}

static void control_panel_render_manual_control(void)
{
    CLAY(CLAY_ID("MANUAL_CONTROL_ENABLE_WRAPPER"),
    {
        .layout = 
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .sizing = { .width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_ENABLE_MANUAL_CONTROL),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("ManuelEnableCheckboxSpacer"),
        {
            .layout = 
            { 
                .sizing = { .width = CLAY_SIZING_GROW(0) } 
            }
        }) {}
        checkbox_render(&enable_manual_control);
    }

    CLAY(CLAY_ID("MANUAL_NUMERIC_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_LABEL_PWM),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("ManualInputSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&manual_pwm_numeric);
    }

	return;
}

static void control_panel_render_two_point_control(void)
{
    CLAY(CLAY_ID("TWO_POINT_CONTROL_ENABLE_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_ENABLE_TWO_POINT_CONTROL),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });

        CLAY(CLAY_ID("TwoPointEnableCheckboxSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        checkbox_render(&enable_two_point_control);
    }

    CLAY(CLAY_ID("SET_POINT_TWO_POINT_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_LABEL_SET_POINT),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("SetPointTwoPointSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&set_point_two_point_numeric);
    }

    CLAY(CLAY_ID("HYSTERESE_TWO_POINT_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_LABEL_HYSTERESE),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("HystereseTwoPointSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&hysterese_two_point_numeric);
    }

    return;
}

static void control_panel_render_auto_control(void)
{
    CLAY(CLAY_ID("AUTO_CONTROL_ENABLE_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
            .sizing = { .width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_ENABLE_AUTO_CONTROL),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("AutoEnableCheckboxSpacer"),
        {
            .layout =
            {
                .sizing = { .width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        checkbox_render(&enable_auto_control);
    }

    CLAY(CLAY_ID("SET_POINT_PID_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_LABEL_SET_POINT),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("SetPointPIDSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&set_point_pid_numeric);
    }

    CLAY(CLAY_ID("ENABLE_P_TERM_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_P_TERM),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("PTermCheckboxSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        checkbox_render(&enable_p_term);
    }

    CLAY(CLAY_ID("PID_KP_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .padding = {.left = CONTROL_PANEL_VARIABLE_LEFT_PADDING }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_P_VARIABLE),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("PIDKpSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&kp_numeric);
    }

    CLAY(CLAY_ID("ENABLE_I_TERM_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_I_TERM),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("ITermCheckboxSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        checkbox_render(&enable_i_term);
    }

    CLAY(CLAY_ID("PID_KI_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .padding = {.left = CONTROL_PANEL_VARIABLE_LEFT_PADDING }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_I_VARIABLE),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("PIDKiSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&ki_numeric);
    }

    CLAY(CLAY_ID("ENABLE_D_TERM_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_D_TERM),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("DTermCheckboxSpacer"),
        {
            .layout =
            {
                .sizing = { .width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        checkbox_render(&enable_d_term);
    }

    CLAY(CLAY_ID("PID_KD_WRAPPER"),
    {
        .layout =
        {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {.width = CLAY_SIZING_GROW(0) },
            .padding = { .left = CONTROL_PANEL_VARIABLE_LEFT_PADDING }
        }
    })
    {
        CLAY_TEXT(get_label(STRING_ID_D_VARIABLE),
        {
            .fontId = font_id(BASE_FONT_FAMILY, BASE_FONT_SIZE),
            .fontSize = get_font_size_int(BASE_FONT_SIZE),
            .textColor = get_color(APP_COLOR_TEXT)
        });
        CLAY(CLAY_ID("PIDKdSpacer"),
        {
            .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0) }
            }
        }) {}
        numeric_input_render(&kd_numeric);
    }
    return;
}

static const char* control_panel_get_mode_string(const bridge_control_mode_t mode)
{
    switch (mode)
    {
    case BRIDGE_CONTROL_MODE_NONE: return "NONE";
    case BRIDGE_CONTROL_MODE_MANUAL: return "MANUAL";
    case BRIDGE_CONTROL_MODE_TWO_POINT: return "TWO_POINT";
    case BRIDGE_CONTROL_MODE_PID: return "PID";
    default: return "";
    }
    return "";
}

static void control_panel_log_config_change(const bridge_control_config_t* new_config)
{
    /* Check arguments */
    if (new_config == NULL) return;

    /* Check if config changed */
    if (memcmp(new_config, &last_control_config, sizeof(last_control_config)) == 0) return;

    char message[128];

    /* Mode change */
    if (new_config->mode != last_control_config.mode)
    {
        const char* new_mode_str = control_panel_get_mode_string(new_config->mode);
        const char* old_mode_str = control_panel_get_mode_string(last_control_config.mode);
        snprintf(message, sizeof(message), "Switched from mode [%s] to [%s]", old_mode_str, new_mode_str);
        bridge_log_queue_push(message);
    }

    /* Manual mode change */
    if (new_config->manual_pwm_percent != last_control_config.manual_pwm_percent)
    {
        snprintf(message, sizeof(message), "Switched manual pwm value from %.0f%% to %.0f%%", last_control_config.manual_pwm_percent, new_config->manual_pwm_percent);
        bridge_log_queue_push(message);
    }

    /* Two point control change */
    if (new_config->two_point_setpoint_c != last_control_config.two_point_setpoint_c)
    {
        snprintf(message, sizeof(message), "Switched set point for two point control from %.1f °C to %.1f °C", last_control_config.two_point_setpoint_c, new_config->two_point_setpoint_c);
        bridge_log_queue_push(message);
    }
    if (new_config->two_point_hysteresis_c != last_control_config.two_point_hysteresis_c)
    {
        snprintf(message, sizeof(message), "Switched hysteresis for two point control from %.1f °C to %.1f °C", last_control_config.two_point_hysteresis_c, new_config->two_point_hysteresis_c);
        bridge_log_queue_push(message);
    }

    /* PID - set point */
    if (new_config->pid_setpoint_c != last_control_config.pid_setpoint_c)
    {
        snprintf(message, sizeof(message), "Switched set point for PID control from %.1f °C to %.1f °C", last_control_config.pid_setpoint_c, new_config->pid_setpoint_c);
        bridge_log_queue_push(message);
    }

    /* PID - term variables */
    if (new_config->pid_kp != last_control_config.pid_kp)
    {
        snprintf(message, sizeof(message), "Switched Kp from %.2f to %.2f", last_control_config.pid_kp, new_config->pid_kp);
        bridge_log_queue_push(message);
    }
    if (new_config->pid_ki != last_control_config.pid_ki)
    {
        snprintf(message, sizeof(message), "Switched Ki from %.2f to %.2f", last_control_config.pid_ki, new_config->pid_ki);
        bridge_log_queue_push(message);
    }
    if (new_config->pid_kd != last_control_config.pid_kd)
    {
        snprintf(message, sizeof(message), "Switched Kd from %.2f to %.2f", last_control_config.pid_kd, new_config->pid_kd);
        bridge_log_queue_push(message);
    }

    /* PID - which part is enabled */
    if (new_config->pid_p_enabled != last_control_config.pid_p_enabled)
    {
        snprintf(message, sizeof(message), "P term %s", new_config->pid_p_enabled ? "enabled" : "disabled");
        bridge_log_queue_push(message);
    }
    if (new_config->pid_i_enabled != last_control_config.pid_i_enabled)
    {
        snprintf(message, sizeof(message), "I term %s", new_config->pid_i_enabled ? "enabled" : "disabled");
        bridge_log_queue_push(message);
    }
    if (new_config->pid_d_enabled != last_control_config.pid_d_enabled)
    {
        snprintf(message, sizeof(message), "D term %s", new_config->pid_d_enabled ? "enabled" : "disabled");
        bridge_log_queue_push(message);
    }

    last_control_config = *new_config;
    return;
}

static void control_panel_publish_control_config(void)
{
    static bridge_control_config_t config = { 0 };

    /* Get control mode */
    if (enable_manual_control.checked)
    {
        config.mode = BRIDGE_CONTROL_MODE_MANUAL;
    }
    else if (enable_two_point_control.checked)
    {
        config.mode = BRIDGE_CONTROL_MODE_TWO_POINT;
    }
    else if (enable_auto_control.checked)
    {
        config.mode = BRIDGE_CONTROL_MODE_PID;
    }
    else
    {
        config.mode = BRIDGE_CONTROL_MODE_NONE;
    }

    /* Get manual pwm value */
    config.manual_pwm_percent = manual_pwm_numeric.value;

    /* Get two point control */
    config.two_point_setpoint_c = set_point_two_point_numeric.value;
    config.two_point_hysteresis_c = hysterese_two_point_numeric.value;

    /* Get pid control */
    config.pid_setpoint_c = set_point_pid_numeric.value;
    config.pid_kp = kp_numeric.value;
    config.pid_ki = ki_numeric.value;
    config.pid_kd = kd_numeric.value;
    config.pid_p_enabled = enable_p_term.checked;
    config.pid_i_enabled = enable_i_term.checked;
    config.pid_d_enabled = enable_d_term.checked;

    /* Publish config */
    bridge_publish_control_config(&config);

    /* Log if something changed */
    control_panel_log_config_change(&config);
    return;
}

static void control_panel_handle_baud(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    int32_t baud_rate = (int32_t)(intptr_t)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        com_info.baud_rate = baud_rate;
    }
    return;
}

static void control_panel_handle_reset_com(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        memset(&com_info, 0, sizeof(com_info));
    }
    return;
}

static void control_panel_handle_com_connect(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        bridge_publish_connection_request(com_info.port_number, com_info.baud_rate, com_connect_button.toggled_state, false);
    }
    return;
}


static void control_panel_handle_auto_connect(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    (void)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        bridge_publish_connection_request(com_info.port_number, com_info.baud_rate, true, true);
    }
    return;
}

static void control_panel_handle_enable_manual(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    bool checked = *(bool*)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        if (checked)
        {
            enable_two_point_control.checked = false;
            enable_auto_control.checked = false;
        }
    }
    return;
}

static void control_panel_handle_enable_two_point(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    bool checked = *(bool*)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        if (checked)
        {
            enable_manual_control.checked = false;
            enable_auto_control.checked = false;
        }
    }
    return;
}

static void control_panel_handle_enable_auto(Clay_ElementId element_id, Clay_PointerData data, void* user_data)
{
    (void)element_id;
    bool checked = *(bool*)user_data;

    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    {
        if (checked)
        {
            enable_manual_control.checked = false;
            enable_two_point_control.checked = false;
        }
    }
    return;
}

/* Function definition */
void control_panel_init(void)
{
    /* Initialize dropdown for com port selection */
    memset(&current_list, 0, sizeof(current_list));
    memset(&com_ports, 0, sizeof(com_ports));
    for (size_t port_index = 0; port_index < (COM_PORT_NUM_MAX - 1); port_index++)
    {
        dropdown_item_t* current_port = &com_ports[port_index];
        current_port->callback = NULL;
        current_port->user_data = NULL;
    }
    dropdown_register_menu(com_ports, COM_PORT_NUM_MAX - 1);

    /* Initialize dropdown for baud selection */
    memset(&baud_rates, 0, sizeof(baud_rates));
    for (size_t rate_index = 0; rate_index < (sizeof(baud_rates) / sizeof(baud_rates[0])); rate_index++)
    {
        dropdown_item_t* current_rate = &baud_rates[rate_index];
        int32_t rate_num = possible_baud_rates[rate_index];
        snprintf(current_rate->raw_label, sizeof(current_rate->raw_label), "%d", rate_num);
        current_rate->use_raw_label = true;
        current_rate->callback = control_panel_handle_baud;
        current_rate->user_data = (void*)(intptr_t)rate_num;
    }
    dropdown_register_menu(baud_rates, sizeof(baud_rates) / sizeof(baud_rates[0]));

    /* Initialize current port information */
    memset(&com_info, 0, sizeof(com_info));

    /* Initialize com buttons */
    memset(&com_reset_button, 0, sizeof(com_reset_button));
    button_init(&com_reset_button);
    com_reset_button.labels.enabled_label = STRING_ID_RESET;
    com_reset_button.labels.disabled_label = STRING_ID_RESET;
    com_reset_button.enabled = false;
    com_reset_button.is_toggle_button = false;
    com_reset_button.callback = control_panel_handle_reset_com;

    memset(&com_connect_button, 0, sizeof(com_connect_button));
    button_init(&com_connect_button);
    com_connect_button.labels.toggle_on_label = STRING_ID_DISCONNECT;
    com_connect_button.labels.toggle_off_label = STRING_ID_CONNECT;
    com_connect_button.enabled = true;
    com_connect_button.is_toggle_button = true;
    com_connect_button.toggled_state = false;
    com_connect_button.callback = control_panel_handle_com_connect;
    com_connect_button.color_override.toggle_off_color = &connect_idle_color;
    com_connect_button.color_override.toggle_on_color = &connect_active_color;

    memset(&com_auto_connect_button, 0, sizeof(com_auto_connect_button));
    button_init(&com_auto_connect_button);
    com_auto_connect_button.labels.enabled_label = STRING_ID_AUTO_CONNECT;
    com_auto_connect_button.labels.disabled_label = STRING_ID_AUTO_CONNECT;
    com_auto_connect_button.enabled = true;
    com_auto_connect_button.is_toggle_button = false;
    com_auto_connect_button.callback = control_panel_handle_auto_connect;

    /* Initialize manual control elements */
	enable_manual_control.enabled = true;
    enable_manual_control.callback = control_panel_handle_enable_manual;
    enable_manual_control.user_data = &enable_manual_control.checked;
	checkbox_init(&enable_manual_control);

    enable_two_point_control.enabled = true;
    enable_two_point_control.callback = control_panel_handle_enable_two_point;
    enable_two_point_control.user_data = &enable_two_point_control.checked;
    checkbox_init(&enable_two_point_control);

    enable_auto_control.enabled = true;
    enable_auto_control.callback = control_panel_handle_enable_auto;
    enable_auto_control.user_data = &enable_auto_control.checked;
    checkbox_init(&enable_auto_control);

    enable_p_term.enabled = true;
    checkbox_init(&enable_p_term);

    enable_i_term.enabled = true;
    checkbox_init(&enable_i_term);

    enable_d_term.enabled = true;
    checkbox_init(&enable_d_term);

    /* Initialize numeric inputs */
    manual_pwm_numeric.focused = false;
    manual_pwm_numeric.allow_decimal = false;
    manual_pwm_numeric.max = CONTROL_PANEL_MANUAL_NUMERIC_PWM_MAX;
    manual_pwm_numeric.min = CONTROL_PANEL_MANUAL_NUMERIC_PWM_MIN;
    manual_pwm_numeric.value = 0.0f;
    numeric_input_init(&manual_pwm_numeric);

    set_point_two_point_numeric.focused = false;
    set_point_two_point_numeric.allow_decimal = true;
    set_point_two_point_numeric.decimal_places = 1;
    set_point_two_point_numeric.min = CONTROL_PANEL_SET_POINT_MIN;
    set_point_two_point_numeric.max = CONTROL_PANEL_SET_POINT_MAX;
    set_point_two_point_numeric.value = 0.0f;
    numeric_input_init(&set_point_two_point_numeric);

    hysterese_two_point_numeric.focused = false;
    hysterese_two_point_numeric.allow_decimal = true;
    hysterese_two_point_numeric.decimal_places = 1;
    hysterese_two_point_numeric.min = CONTROL_PANEL_HYSTERESE_MIN;
    hysterese_two_point_numeric.max = CONTROL_PANEL_HYSTERESE_MAX;
    hysterese_two_point_numeric.value = 0.0f;
    numeric_input_init(&hysterese_two_point_numeric);

    set_point_pid_numeric.focused = false;
    set_point_pid_numeric.allow_decimal = true;
    set_point_pid_numeric.decimal_places = 1;
    set_point_pid_numeric.min = CONTROL_PANEL_SET_POINT_MIN;
    set_point_pid_numeric.max = CONTROL_PANEL_SET_POINT_MAX;
    set_point_pid_numeric.value = 0.0f;
    numeric_input_init(&set_point_pid_numeric);

    kp_numeric.focused = false;
    kp_numeric.allow_decimal = true;
    kp_numeric.decimal_places = 2;
    kp_numeric.min = CONTROL_PANEL_PID_MIN;
    kp_numeric.max = CONTROL_PANEL_PID_MAX;
    kp_numeric.value = 0.0f;
    numeric_input_init(&kp_numeric);

    ki_numeric.focused = false;
    ki_numeric.allow_decimal = true;
    ki_numeric.decimal_places = 2;
    ki_numeric.min = CONTROL_PANEL_PID_MIN;
    ki_numeric.max = CONTROL_PANEL_PID_MAX;
    ki_numeric.value = 0.0f;
    numeric_input_init(&ki_numeric);

    kd_numeric.focused = false;
    kd_numeric.allow_decimal = true;
    kd_numeric.decimal_places = 2;
    kd_numeric.min = CONTROL_PANEL_PID_MIN;
    kd_numeric.max = CONTROL_PANEL_PID_MAX;
    kd_numeric.value = 0.0f;
    numeric_input_init(&kd_numeric);

    bridge_get_control_config(&last_control_config);

    return;
}

void control_panel_apply_imported_config(const bridge_control_config_t* config)
{
    if (config == NULL) return;

    manual_pwm_numeric.value = config->manual_pwm_percent;
    set_point_two_point_numeric.value = config->two_point_setpoint_c;
    hysterese_two_point_numeric.value = config->two_point_hysteresis_c;
    set_point_pid_numeric.value = config->pid_setpoint_c;
    kp_numeric.value = config->pid_kp;
    ki_numeric.value = config->pid_ki;
    kd_numeric.value = config->pid_kd;
    enable_manual_control.checked = (config->mode == BRIDGE_CONTROL_MODE_MANUAL);
    enable_two_point_control.checked = (config->mode == BRIDGE_CONTROL_MODE_TWO_POINT);
    enable_auto_control.checked = (config->mode == BRIDGE_CONTROL_MODE_PID);
    enable_p_term.checked = config->pid_p_enabled;
    enable_i_term.checked = config->pid_i_enabled;
    enable_d_term.checked = config->pid_d_enabled;

    numeric_input_update(&manual_pwm_numeric);
    numeric_input_update(&set_point_two_point_numeric);
    numeric_input_update(&hysterese_two_point_numeric);
    numeric_input_update(&set_point_pid_numeric);
    numeric_input_update(&kp_numeric);
    numeric_input_update(&ki_numeric);
    numeric_input_update(&kd_numeric);

    return;
}

void control_panel_render(void)
{
    CLAY(CLAY_ID("ControlPanelWrapper"),
    {
        .layout =
        {
            .sizing = {.width = CONTROL_PANEL_WIDTH, .height = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = get_color(APP_COLOR_BACKGROUND),
    })
    {

        CLAY(CLAY_ID("ConnectionWrapper"),
        {
            .layout =
            {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = HALF_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = { .topLeft = STANDARD_RADIUS, .topRight = STANDARD_RADIUS },
        })
        {
            control_panel_render_com_port_list();
            control_panel_render_baud_list();

            CLAY(CLAY_ID("ComSpacer"),
            {
                .layout = { .sizing = { .height = CLAY_SIZING_GROW(0) } }
            }) {}

            control_panel_render_com_buttons();
        }

        CLAY(CLAY_ID("ManualWrapper"),
        {
            .layout =
            {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = HALF_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .border =
            {
                .color = get_color(APP_COLOR_PANEL_OUTLINE),
                .width = { .top = CONTROL_PANEL_LINE_WIDTH }
            }
        })
        {
            control_panel_render_manual_control();
        }

        CLAY(CLAY_ID("TwoPointWrapper"),
        {
            .layout =
            {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = HALF_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .border =
            {
                .color = get_color(APP_COLOR_PANEL_OUTLINE),
                .width = { .bottom = CONTROL_PANEL_LINE_WIDTH, .top = CONTROL_PANEL_LINE_WIDTH }
            }
        })
        {
            control_panel_render_two_point_control();
        }

        CLAY(CLAY_ID("PIDWrapper"),
        {
            .layout =
            {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(STANDARD_PADDING),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = HALF_PADDING
            },
            .backgroundColor = get_color(APP_COLOR_BOX_BACKGROUND),
            .cornerRadius = { .bottomLeft = STANDARD_RADIUS, .bottomRight = STANDARD_RADIUS },
        })
        {
            control_panel_render_auto_control();
        }
    }

    control_panel_publish_control_config();
    return;
}