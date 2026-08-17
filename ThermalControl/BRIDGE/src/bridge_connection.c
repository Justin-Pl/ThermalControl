/* Header */
#include "bridge_connection.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Static local variables */
static connection_info_t current_connection_info = { .baud_rate = -1, .connected = false, .port_number = -1 };
static CRITICAL_SECTION connection_info_lock;
static com_port_list_t port_list = { .com_count = 0, .refresh_count = 0 };
static CRITICAL_SECTION port_list_lock;
static connection_request_t current_request = { .port_number = 0, .baud_rate = 0, .connect_requested = false, .refresh_count = 0 };
static CRITICAL_SECTION request_lock;

/* Static function definition */
static bool bridge_lists_are_equal(const com_port_list_t* new_list)
{
    if (new_list == NULL) return false;
    if (port_list.com_count != new_list->com_count) return false;

    int safe_count = new_list->com_count;
    if (safe_count > (int)(sizeof(port_list.com_numbers) / sizeof(port_list.com_numbers[0])))
    {
        safe_count = (int)(sizeof(port_list.com_numbers) / sizeof(port_list.com_numbers[0]));
    }

    for (int com_index = 0; com_index < safe_count; com_index++)
    {
        uint16_t port_num = port_list.com_numbers[com_index];
        uint16_t port_num_new_list = new_list->com_numbers[com_index];
        if (port_num != port_num_new_list) return false;
    }
    return true;
}

static bool bridge_requests_are_equal(uint16_t port_number, int32_t baud_rate, bool connect, bool auto_connect)
{
    return (current_request.port_number == port_number)
        && (current_request.baud_rate == baud_rate)
        && (current_request.connect_requested == connect)
        && (current_request.auto_connect == auto_connect);
}

/* Function definitions */
void bridge_connection_init(void)
{
    InitializeCriticalSection(&connection_info_lock);
    InitializeCriticalSection(&port_list_lock);
    InitializeCriticalSection(&request_lock);

    return;
}

void bridge_publish_connection_request(uint16_t port_number, int32_t baud_rate, bool connect, bool auto_connect)
{
    EnterCriticalSection(&request_lock);
    if (!bridge_requests_are_equal(port_number, baud_rate, connect, auto_connect))
    {
        current_request.port_number = port_number;
        current_request.baud_rate = baud_rate;
        current_request.connect_requested = connect;
        current_request.auto_connect = auto_connect;
        current_request.refresh_count++;
    }
    LeaveCriticalSection(&request_lock);
    return;
}

bool bridge_get_connection_request_if_changed(connection_request_t* out)
{
    if (out == NULL) return false;
    bool changed = false;
    EnterCriticalSection(&request_lock);
    if (current_request.refresh_count != out->refresh_count)
    {
        *out = current_request;
        changed = true;
    }
    LeaveCriticalSection(&request_lock);
    return changed;
}

void bridge_publish_connection_info(bool connected, int16_t port_number, int32_t baud_rate, bool auto_connect_running)
{
    EnterCriticalSection(&connection_info_lock);
    current_connection_info.connected = connected;
    current_connection_info.port_number = port_number;
    current_connection_info.baud_rate = baud_rate;
    current_connection_info.auto_connect_running = auto_connect_running;
    LeaveCriticalSection(&connection_info_lock);

    return;
}

bool bridge_get_connection_info(connection_info_t* out)
{
    if (out == NULL) return false;

    EnterCriticalSection(&connection_info_lock);
    *out = current_connection_info;
    LeaveCriticalSection(&connection_info_lock);
    return true;
}

void bridge_publish_com_ports(const com_port_list_t* new_list)
{
    if (new_list == NULL) return;

    int safe_count = new_list->com_count;
    if (safe_count > (int)(sizeof(port_list.com_numbers) / sizeof(port_list.com_numbers[0])))
    {
        safe_count = (int)(sizeof(port_list.com_numbers) / sizeof(port_list.com_numbers[0]));
    }

    EnterCriticalSection(&port_list_lock);
    if (!bridge_lists_are_equal(new_list))
    {
        size_t copy_size = safe_count * sizeof(new_list->com_numbers[0]);
        memcpy(&port_list, new_list, copy_size);
        port_list.com_count = safe_count;
        port_list.refresh_count++;   
    }
    LeaveCriticalSection(&port_list_lock);
}

bool bridge_get_com_ports_if_changed(com_port_list_t* out)
{
    if (out == NULL) return false;
    bool changed = false;

    EnterCriticalSection(&port_list_lock);
    if (port_list.refresh_count != out->refresh_count)
    {
        *out = port_list;
        changed = true;
    }
    LeaveCriticalSection(&port_list_lock);

    return changed;
}