/* Header */
#include "worker_com_list.h"

/* Libraries */
#include <windows.h>

/* Defines */
#define COM_LIST_THREAD_INTERVAL_MS        1000
#define COM_LIST_THREAD_STOP_TIMEOUT       2000

/* Static local variables */
static HANDLE thread_handle = NULL;
static volatile LONG should_stop = 0;

/* Static function definition */
bool data_scan_serial_ports(uint16_t* out_numbers, int max_count, uint8_t* out_count)
{
    *out_count = 0;

    HKEY key;
    LONG open_result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key);
    if (open_result != ERROR_SUCCESS) return true;

    int found_count = 0;
    DWORD value_index = 0;

    while (found_count < max_count)
    {
        char value_name[256];
        char value_data[256];
        DWORD value_name_size = sizeof(value_name);
        DWORD value_data_size = sizeof(value_data);
        DWORD value_type;

        LONG enum_result = RegEnumValueA(key, value_index, value_name, &value_name_size,
            NULL, &value_type, (LPBYTE)value_data, &value_data_size);

        if (enum_result == ERROR_NO_MORE_ITEMS) break;

        if ((enum_result == ERROR_SUCCESS) && (value_type == REG_SZ))
        {
            char* digits = value_data;
            while ((*digits != '\0') && !isdigit((unsigned char)*digits)) digits++;

            if (*digits != '\0')
            {
                int port_num = atoi(digits);
                if ((port_num >= COM_PORT_NUM_MIN) && (port_num <= COM_PORT_NUM_MAX))
                {
                    out_numbers[found_count++] = (uint16_t)port_num;
                }
            }
        }

        value_index++;
    }

    RegCloseKey(key);
    *out_count = found_count;
    return true;
}

static DWORD WINAPI test_graph_backend_thread_proc(LPVOID param)
{
    (void)param;

    static com_port_list_t com_list;
    memset(&com_list, 0, sizeof(com_list));
    while (InterlockedCompareExchange(&should_stop, 0, 0) == 0)
    {
        data_scan_serial_ports(com_list.com_numbers, COM_PORT_NUM_MAX - 1, &com_list.com_count);

        bridge_publish_com_ports(&com_list);
        Sleep(COM_LIST_THREAD_INTERVAL_MS);   
    }

    return 0;
}

/* Function definition */
void worker_com_list_start(void)
{
    should_stop = 0;
    thread_handle = CreateThread(NULL, 0, test_graph_backend_thread_proc, NULL, 0, NULL);
	return;
}

void worker_com_list_stop(void)
{
    InterlockedExchange(&should_stop, 1);

    if (thread_handle != NULL)
    {
        WaitForSingleObject(thread_handle, COM_LIST_THREAD_STOP_TIMEOUT);
        CloseHandle(thread_handle);
        thread_handle = NULL;
    }
	return;
}