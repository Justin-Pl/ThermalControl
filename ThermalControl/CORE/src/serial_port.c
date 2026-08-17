/* Header */
#include "serial_port.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Defines */
#define SERIAL_PORT_BYTE_SIZE		8
#define SERIAL_PORT_PARITY			NOPARITY
#define SERIAL_PORT_STOP_BITS		ONESTOPBIT

/* Static local variables */
static volatile LONG current_owner = PORT_OWNER_NONE;
static HANDLE port_handle = INVALID_HANDLE_VALUE;
static HANDLE ownership_mutex = NULL;

/* Function definitions */
void serial_port_init(void)
{
	ownership_mutex = CreateMutexA(NULL, FALSE, NULL);
	return;
}

bool serial_port_flush(const serial_port_owner_t self)
{
	if ((LONG)self != current_owner) return false;
	if (port_handle == INVALID_HANDLE_VALUE) return false;

	return PurgeComm(port_handle, PURGE_RXCLEAR) != 0;
}

bool serial_port_open(const serial_port_owner_t self, const char* port, const uint32_t baud)
{
	/* Check arguments */
	if ((LONG)self != current_owner) return false;
	if (port_handle != INVALID_HANDLE_VALUE) return false;
	if (port == NULL) return false;
	if (!baud) return false;

	/* Open serial port in read/write config */
	char path[16];
	snprintf(path, sizeof(path), "\\\\.\\%s", port);
	port_handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (port_handle == INVALID_HANDLE_VALUE) return false;

	/* Get current port config */
	DCB communication_config = { 0 };
	communication_config.DCBlength = sizeof(communication_config);

	communication_config.BaudRate = baud;
	communication_config.ByteSize = SERIAL_PORT_BYTE_SIZE;
	communication_config.Parity = SERIAL_PORT_PARITY;
	communication_config.StopBits = SERIAL_PORT_STOP_BITS;

	communication_config.fBinary = TRUE;              /* zwingend TRUE fuer Win32, sonst undefiniert */
	communication_config.fParity = FALSE;
	communication_config.fOutxCtsFlow = FALSE;
	communication_config.fOutxDsrFlow = FALSE;
	communication_config.fDtrControl = DTR_CONTROL_ENABLE;
	communication_config.fDsrSensitivity = FALSE;     /* DER entscheidende Fix */
	communication_config.fTXContinueOnXoff = TRUE;
	communication_config.fOutX = FALSE;
	communication_config.fInX = FALSE;
	communication_config.fErrorChar = FALSE;
	communication_config.fNull = FALSE;
	communication_config.fRtsControl = RTS_CONTROL_ENABLE;
	communication_config.fAbortOnError = FALSE;

	if (!SetCommState(port_handle, &communication_config))
	{
		CloseHandle(port_handle);
		port_handle = INVALID_HANDLE_VALUE;
		return false;
	}

	return true;
}

size_t serial_port_write(const serial_port_owner_t self, const uint8_t* data, const size_t length)
{
	/* Check arguments */
	if ((LONG)self != current_owner) return 0;
	if ((data == NULL) || !length) return 0;
	if (port_handle == INVALID_HANDLE_VALUE) return 0;

	/* Write to port */
	DWORD written = 0;
	WriteFile(port_handle, data, (DWORD)length, &written, NULL);

	return (size_t)written;
}
size_t serial_port_read(const serial_port_owner_t self, uint8_t* buffer, const size_t buffer_size, const uint32_t timeout_ms)
{
	/* Check arguments */
	if ((LONG)self != current_owner) return 0;
	if ((buffer == NULL) || !buffer_size) return 0;
	if (port_handle == INVALID_HANDLE_VALUE) return 0;

	/* Set timeout */
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = timeout_ms;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	SetCommTimeouts(port_handle, &timeouts);

	/* Read from port */
	DWORD bytes_read = 0;
	BOOL result = ReadFile(port_handle, buffer, (DWORD)buffer_size, &bytes_read, NULL);
	if (!result)
	{
		printf("[DEBUG] ReadFile fehlgeschlagen, GetLastError=%lu\n", GetLastError());
	}

	return (size_t)bytes_read;
}

bool serial_port_close(const serial_port_owner_t self)
{
	/* Check arguments */
	if ((LONG)self != current_owner) return false;
	if (port_handle == INVALID_HANDLE_VALUE) return false;

	/* Close handler */
	CloseHandle(port_handle);
	port_handle = INVALID_HANDLE_VALUE;

	return true;
}

bool serial_port_acquire(const serial_port_owner_t self, const uint32_t timeout_ms)
{
	/* Check arguments & if self is already owner */
	if ((LONG)self == current_owner) return true;

	/* Wait for mutex */
	DWORD result = WaitForSingleObject(ownership_mutex, timeout_ms);
	if (result != WAIT_OBJECT_0) return false;

	/* Change owner */
	InterlockedExchange(&current_owner, (LONG)self);
	return true;
}

bool serial_port_release(const serial_port_owner_t self)
{
	if ((LONG)self != current_owner) return false;
	if (port_handle != INVALID_HANDLE_VALUE) return false;

	/* Reset owner */
	InterlockedExchange(&current_owner, (LONG)PORT_OWNER_NONE);
	ReleaseMutex(ownership_mutex);

	return true;
}

const serial_port_owner_t serial_port_owner(void)
{
	return (serial_port_owner_t)InterlockedCompareExchange(&current_owner, 0, 0);
}