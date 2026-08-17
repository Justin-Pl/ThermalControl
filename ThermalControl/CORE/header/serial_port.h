#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

/* Libraries */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Type definitions */
typedef enum
{
	PORT_OWNER_NONE,
	PORT_FLASHING,
	PORT_SENSOR
} serial_port_owner_t; 

/* Function declarations */
void serial_port_init(void);
bool serial_port_flush(const serial_port_owner_t self);
bool serial_port_open(const serial_port_owner_t self, const char* port, const uint32_t baud);
size_t serial_port_write(const serial_port_owner_t self, const uint8_t* data, const size_t length);
size_t serial_port_read(const serial_port_owner_t self, uint8_t* buffer, const size_t buffer_size, const uint32_t timeout_ms);
bool serial_port_close(const serial_port_owner_t self);
bool serial_port_acquire(const serial_port_owner_t self, const uint32_t timeout_ms);
bool serial_port_release(const serial_port_owner_t self);
const serial_port_owner_t serial_port_owner(void);

#endif // SERIAL_PORT_H
