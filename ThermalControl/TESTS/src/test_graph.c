/* Header */
#include "test_graph.h"

/* Libraries */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <time.h>
#include "bridge_graph_queue.h"

static volatile LONG queue_should_stop = 0;
static HANDLE thread_handle = NULL;

static uint64_t get_current_utc_ms(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    uint64_t ms_since_1601 = uli.QuadPart / 10000ULL;
    const uint64_t EPOCH_DIFF_MS = 11644473600000ULL;
    return ms_since_1601 - EPOCH_DIFF_MS;
}

static DWORD WINAPI test_graph_backend_thread_proc(LPVOID param)
{
    (void)param;
    double t = 0.0;

    while (InterlockedCompareExchange(&queue_should_stop, 0, 0) == 0)
    {
        graph_point_t point;
        point.timestamp_ms = get_current_utc_ms();
        point.temp = 22.0f + 5.0f * (float)sin(t * 0.1);
        point.pwm = 50.0f + 30.0f * (float)sin(t * 0.05);

        bridge_graph_queue_push(point);

        t += 2.0;
        Sleep(2000);   /* 5 Punkte pro Sekunde */
    }

    return 0;
}

void test_graph_backend_start(void)
{
    queue_should_stop = 0;
    thread_handle = CreateThread(NULL, 0, test_graph_backend_thread_proc, NULL, 0, NULL);
    return;
}

void test_graph_backend_stop(void)
{
    InterlockedExchange(&queue_should_stop, 1);

    if (thread_handle != NULL)
    {
        WaitForSingleObject(thread_handle, 2000);   /* bis zu 2s auf sauberes Ende warten */
        CloseHandle(thread_handle);
        thread_handle = NULL;
    }
    return;
}