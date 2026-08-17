/* Source files */
#include "communication.h"
#include "register.h"
#include "time.h"
#include "temp.h"

/* Setup (called once) */
void setup() 
{
    /* Init internal registers */
    init_registers();

    /* Init serial communication interface */
    init_communication();

    /* Init temperature sensor */
    init_temp_sensor();

    /* Debug LED */
    pinMode(LED_BUILTIN, OUTPUT);
}

/* Main program */
void loop() 
{
    /* Uptime runtime since boot in milliseconds */
    update_uptime();

    /* Update temp value */
    update_temp_sensor();

    /* Check for new serial data */
    poll_serial();

    /* Process new received frames */
    process_frames();
}