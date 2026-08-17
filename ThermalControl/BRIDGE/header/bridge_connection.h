#ifndef BRIDGE_CONNECTION_H
#define BRIDGE_CONNECTION_H

/* Libraries */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Defines */
#define COM_PORT_NAME_MAX			16
#define COM_PORT_NUM_MIN			1
#define COM_PORT_NUM_MAX			256
#define COM_PORT_BAUD_MIN			9600
#define COM_PORT_BAUD_MAX			115200

/* Type definitions */
typedef struct
{
	bool connected;
	bool auto_connect_running;
	int16_t port_number;
	int32_t baud_rate;
} connection_info_t;

typedef struct
{
	uint16_t port_number;
	int32_t baud_rate;
	bool connect_requested;  
	bool auto_connect;
	uint32_t refresh_count;   
} connection_request_t;

typedef struct
{
	uint16_t com_numbers[COM_PORT_NUM_MAX - 1];
	uint8_t com_count;
	uint32_t refresh_count;
} com_port_list_t;

/* Function declaration */
void bridge_connection_init(void);
void bridge_publish_connection_request(uint16_t port_number, int32_t baud_rate, bool connect, bool auto_connect);
bool bridge_get_connection_request_if_changed(connection_request_t* out);
void bridge_publish_connection_info(bool connected, int16_t port_number, int32_t baud_rate, bool auto_connect_running);
bool bridge_get_connection_info(connection_info_t* out);
void bridge_publish_com_ports(const com_port_list_t* new_list);
bool bridge_get_com_ports_if_changed(com_port_list_t* out);

#endif // BRIDGE_CONNECTION_H