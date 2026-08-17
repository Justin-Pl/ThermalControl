/* Header */
#include "test_console.h"

/* Defines */
#define MOCK_QUEUE_CAPACITY			64

/* Type definitions */
typedef struct
{
	console_message_t messages[MOCK_QUEUE_CAPACITY];
	size_t write_position;
	size_t read_position;
    size_t unread_count;
} mock_queue_t;

/* Static variables */
static mock_queue_t queue_a;
static mock_queue_t queue_b;

/* Static function definitions */
static void mock_queue_push(mock_queue_t* queue, const char* text, uint64_t timestamp_ms)
{
    /* Check if arguments are valid */
    if (queue == NULL) return;
    if (text == NULL) return;

    /* Check if queue is full */
    if (queue->unread_count >= MOCK_QUEUE_CAPACITY) return;

    console_message_t* msg = &queue->messages[queue->write_position];
    snprintf(msg->plain_text, CONSOLE_MAX_MESSAGE_LENGTH, "%s", text);
    msg->timestamp_ms = timestamp_ms;

    queue->write_position = (queue->write_position + 1) % MOCK_QUEUE_CAPACITY;
    queue->unread_count++;
    return;
}

static bool mock_queue_pull(mock_queue_t* queue, console_message_t* out)
{
    if (out == NULL) return false;
    if (queue == NULL) return false;
    if (queue->unread_count == 0) return false;

    *out = queue->messages[queue->read_position];
    queue->read_position = (queue->read_position + 1) % MOCK_QUEUE_CAPACITY;
    queue->unread_count--;
    return true;
}

static bool mock_queue_a_pull(console_message_t* out)
{
    return mock_queue_pull(&queue_a, out);
}

static bool mock_queue_b_pull(console_message_t* out)
{
    return mock_queue_pull(&queue_b, out);
}

/* Function definitions */
void test_console_init(void)
{
    /* Initialize mock queues */
    memset(&queue_a, 0, sizeof(mock_queue_t));
    memset(&queue_b, 0, sizeof(mock_queue_t));

    /* Register pull functions */
	console_register_pull_function(mock_queue_a_pull);
    console_register_pull_function(mock_queue_b_pull);

    /* Push test messages */
    mock_queue_push(&queue_b, "[Quelle B] Diese Zeile hat spaeteren Zeitstempel", 200);
    mock_queue_push(&queue_a, "[Quelle A] Diese Zeile hat frueheren Zeitstempel", 100);

	return;
}

void test_console_run(void)
{
    static int counter = 0;
    static double last_push_time = 0;

	double current_time = GetTime();   
    if (current_time - last_push_time < 0.5) return;
    last_push_time = current_time;

    char buf[64];
    uint64_t ts = (uint64_t)(current_time * 1000.0);

    if (counter % 2 == 0)
    {
        snprintf(buf, sizeof(buf), "[Quelle A] Nachricht #%d", counter);
        mock_queue_push(&queue_a, buf, ts);
    }
    else
    {
        snprintf(buf, sizeof(buf), "[Quelle B] Nachricht #%d", counter);
        mock_queue_push(&queue_b, buf, ts);
    }
    counter++;

	return;
}