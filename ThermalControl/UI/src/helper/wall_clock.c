/* Header */
#include "helper/wall_clock.h"

/* Windows lib */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Static local variables */
static uint64_t anchor_tick_ms = 0;
static uint64_t anchor_wall_ms = 0;

/* Static function definition */
static uint64_t get_wall_clock_ms_since_epoch(void)
{
	/* Get the current system time as a FILETIME */
	FILETIME file_time;
	GetSystemTimeAsFileTime(&file_time);

	/* Convert FILETIME to 64-bit integer */
	ULARGE_INTEGER uli;
	uli.LowPart = file_time.dwLowDateTime;
	uli.HighPart = file_time.dwHighDateTime;

	/* FILETIME counts 100-ns intervals since 1.1.1601, subtract difference to Unix epoch */
	uint64_t ms_since_1601 = uli.QuadPart / 10000ULL;
	const uint64_t EPOCH_DIFF_MS = 11644473600000ULL;
	return ms_since_1601 - EPOCH_DIFF_MS;
}

/* Function definition */
void wall_clock_init(uint64_t tick_ms)
{
	anchor_tick_ms = tick_ms;
	anchor_wall_ms = get_wall_clock_ms_since_epoch();
	return;
}

uint64_t wall_clock_convert_from_tick(uint64_t tick_ms)
{
	int64_t offset_ms = (int64_t)tick_ms - (int64_t)anchor_tick_ms;
	return anchor_wall_ms + (uint64_t)offset_ms;
}