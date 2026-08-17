#ifndef BRIDGE_CONTROL_H
#define BRIDGE_CONTROL_H

/* Libraries */
#include <stdbool.h>

/* Type definitions */
typedef enum
{
	BRIDGE_CONTROL_MODE_NONE = 0,
	BRIDGE_CONTROL_MODE_MANUAL,
	BRIDGE_CONTROL_MODE_TWO_POINT,
	BRIDGE_CONTROL_MODE_PID
} bridge_control_mode_t;

typedef struct
{
	bridge_control_mode_t mode;

	/* Manual */
	float manual_pwm_percent;

    /* Two point */
    float two_point_setpoint_c;
    float two_point_hysteresis_c;

    /* PID */
    float pid_setpoint_c;
    float pid_kp;
    float pid_ki;
    float pid_kd;
    bool pid_p_enabled;
    bool pid_i_enabled;
    bool pid_d_enabled;
} bridge_control_config_t;

/* Function declarations */
void bridge_control_init(void);
void bridge_publish_control_config(const bridge_control_config_t* config);
bool bridge_get_control_config(bridge_control_config_t* out);

#endif // BRIDGE_CONTROL_H
