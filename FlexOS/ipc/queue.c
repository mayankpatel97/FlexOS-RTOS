// queue.h
#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "memory.h"

// Queue error codes
typedef enum {
    QUEUE_OK,
    QUEUE_FULL,
    QUEUE_EMPTY,
    QUEUE_ERROR,
    QUEUE_TIMEOUT
} QueueStatus;

// Queue notification type
typedef enum {
    QUEUE_NOTIFY_ON_SEND,
    QUEUE_NOTIFY_ON_RECEIVE,
    QUEUE_NOTIFY_ON_FULL,
    QUEUE_NOTIFY_ON_EMPTY
} QueueNotifyType;

// Queue notification callback
typedef void (*QueueCallback)(void* queue, void* context);

// Queue structure
typedef struct {
    void* buffer;                     // Queue data buffer
    uint32_t item_size;              // Size of each item
    uint32_t queue_length;           // Maximum number of items
    uint32_t items_count;            // Current number of items
    uint32_t head;                   // Read index
    uint32_t tail;                   // Write index
    uint32_t waiting_tasks_send[32]; // Tasks waiting to send
    uint32_t waiting_tasks_recv[32]; // Tasks waiting to receive
    uint32_t waiting_count_send;     // Number of tasks waiting to send
    uint32_t waiting_count_recv;     // Number of tasks waiting to receive
    bool is_isr_enabled;             // ISR usage flag
    QueueCallback notify_callback;    // Notification callback
    void* notify_context;            // Notification context
    QueueNotifyType notify_type;     // Notification type
    uint32_t overflow_count;         // Number of overflow events
    uint32_t underflow_count;        // Number of underflow events
} Queue;

// Queue functions
QueueStatus queue_create(Queue** queue, uint32_t item_size, uint32_t queue_length);
void queue_delete(Queue* queue);
QueueStatus queue_send(Queue* queue, const void* item, uint32_t timeout);
QueueStatus queue_send_from_isr(Queue* queue, const void* item);
QueueStatus queue_receive(Queue* queue, void* buffer, uint32_t timeout);
QueueStatus queue_receive_from_isr(Queue* queue, void* buffer);
QueueStatus queue_peek(Queue* queue, void* buffer);
void queue_reset(Queue* queue);
uint32_t queue_get_count(const Queue* queue);
bool queue_is_full(const Queue* queue);
bool queue_is_empty(const Queue* queue);
void queue_set_notification(Queue* queue, QueueCallback callback, void* context, QueueNotifyType type);
uint32_t queue_get_space_available(const Queue* queue);
QueueStatus queue_send_to_front(Queue* queue, const void* item, uint32_t timeout);
QueueStatus queue_send_to_back(Queue* queue, const void* item, uint32_t timeout);
QueueStatus queue_overwrite(Queue* queue, const void* item);

#endif // RTOS_QUEUE_H

// queue.c
#include "queue.h"
#include "scheduler.h"
#include <string.h>

// Internal helper functions
static inline void copy_to_queue(Queue* queue, const void* item, uint32_t position) {
    uint8_t* write_ptr = (uint8_t*)queue->buffer + (position * queue->item_size);
    memcpy(write_ptr, item, queue->item_size);
}

static inline void copy_from_queue(Queue* queue, void* buffer, uint32_t position) {
    uint8_t* read_ptr = (uint8_t*)queue->buffer + (position * queue->item_size);
    memcpy(buffer, read_ptr, queue->item_size);
}

static void notify_queue_event(Queue* queue, QueueNotifyType event_type) {
    if (queue->notify_callback && queue->notify_type == event_type) {
        queue->notify_callback(queue, queue->notify_context);
    }
}

QueueStatus queue_create(Queue** queue, uint32_t item_size, uint32_t queue_length) {
    if (!queue || item_size == 0 || queue_length == 0) {
        return QUEUE_ERROR;
    }

    // Allocate queue structure
    Queue* new_queue = (Queue*)memory_alloc(sizeof(Queue));
    if (!new_queue) {
        return QUEUE_ERROR;
    }

    // Allocate queue buffer
    void* buffer = memory_alloc(item_size * queue_length);
    if (!buffer) {
        memory_free(new_queue);
        return QUEUE_ERROR;
    }

    // Initialize queue structure
    new_queue->buffer = buffer;
    new_queue->item_size = item_size;
    new_queue->queue_length = queue_length;
    new_queue->items_count = 0;
    new_queue->head = 0;
    new_queue->tail = 0;
    new_queue->waiting_count_send = 0;
    new_queue->waiting_count_recv = 0;
    new_queue->is_isr_enabled = false;
    new_queue->notify_callback = NULL;
    new_queue->notify_context = NULL;
    new_queue->overflow_count = 0;
    new_queue->underflow_count = 0;

    *queue = new_queue;
    return QUEUE_OK;
}

void queue_delete(Queue* queue) {
    if (queue) {
        if (queue->buffer) {
            memory_free(queue->buffer);
        }
        memory_free(queue);
    }
}

QueueStatus queue_send(Queue* queue, const void* item, uint32_t timeout) {
    if (!queue || !item) {
        return QUEUE_ERROR;
    }

    disable_interrupts();

    if (!queue_is_full(queue)) {
        copy_to_queue(queue, item, queue->tail);
        queue->tail = (queue->tail + 1) % queue->queue_length;
        queue->items_count++;

        // Wake up one waiting receiver if any
        if (queue->waiting_count_recv > 0) {
            uint32_t task_to_wake = queue->waiting_tasks_recv[0];

            // Remove task from waiting list
            for (uint32_t i = 0; i < queue->waiting_count_recv - 1; i++) {
                queue->waiting_tasks_recv[i] = queue->waiting_tasks_recv[i + 1];
            }
            queue->waiting_count_recv--;

            resume_task(task_to_wake);
        }

        notify_queue_event(queue, QUEUE_NOTIFY_ON_SEND);

        enable_interrupts();
        return QUEUE_OK;
    }

    if (timeout == 0) {
        queue->overflow_count++;
        enable_interrupts();
        return QUEUE_FULL;
    }

    // Add current task to sending waiting list
    uint32_t current_task = get_current_task_id();
    queue->waiting_tasks_send[queue->waiting_count_send++] = current_task;

    enable_interrupts();
    block_task(timeout);

    return queue_is_full(queue) ? QUEUE_TIMEOUT : QUEUE_OK;
}

QueueStatus queue_send_from_isr(Queue* queue, const void* item) {
    if (!queue || !item || !queue->is_isr_enabled) {
        return QUEUE_ERROR;
    }

    if (!queue_is_full(queue)) {
        copy_to_queue(queue, item, queue->tail);
        queue->tail = (queue->tail + 1) % queue->queue_length;
        queue->items_count++;
        notify_queue_event(queue, QUEUE_NOTIFY_ON_SEND);
        return QUEUE_OK;
    }

    queue->overflow_count++;
    return QUEUE_FULL;
}

QueueStatus queue_receive(Queue* queue, void* buffer, uint32_t timeout) {
    if (!queue || !buffer) {
        return QUEUE_ERROR;
    }

    disable_interrupts();

    if (!queue_is_empty(queue)) {
        copy_from_queue(queue, buffer, queue->head);
        queue->head = (queue->head + 1) % queue->queue_length;
        queue->items_count--;

        // Wake up one waiting sender if any
        if (queue->waiting_count_send > 0) {
            uint32_t task_to_wake = queue->waiting_tasks_send[0];

            // Remove task from waiting list
            for (uint32_t i = 0; i < queue->waiting_count_send - 1; i++) {
                queue->waiting_tasks_send[i] = queue->waiting_tasks_send[i + 1];
            }
            queue->waiting_count_send--;

            resume_task(task_to_wake);
        }

        notify_queue_event(queue, QUEUE_NOTIFY_ON_RECEIVE);

        enable_interrupts();
        return QUEUE_OK;
    }

    if (timeout == 0) {
        queue->underflow_count++;
        enable_interrupts();
        return QUEUE_EMPTY;
    }

    // Add current task to receiving waiting list
    uint32_t current_task = get_current_task_id();
    queue->waiting_tasks_recv[queue->waiting_count_recv++] = current_task;

    enable_interrupts();
    block_task(timeout);

    return queue_is_empty(queue) ? QUEUE_TIMEOUT : QUEUE_OK;
}

QueueStatus queue_receive_from_isr(Queue* queue, void* buffer) {
    if (!queue || !buffer || !queue->is_isr_enabled) {
        return QUEUE_ERROR;
    }

    if (!queue_is_empty(queue)) {
        copy_from_queue(queue, buffer, queue->head);
        queue->head = (queue->head + 1) % queue->queue_length;
        queue->items_count--;
        notify_queue_event(queue, QUEUE_NOTIFY_ON_RECEIVE);
        return QUEUE_OK;
    }

    queue->underflow_count++;
    return QUEUE_EMPTY;
}

void queue_reset(Queue* queue) {
    if (queue) {
        disable_interrupts();
        queue->head = 0;
        queue->tail = 0;
        queue->items_count = 0;
        queue->waiting_count_send = 0;
        queue->waiting_count_recv = 0;
        queue->overflow_count = 0;
        queue->underflow_count = 0;
        enable_interrupts();
    }
}

QueueStatus queue_send_to_front(Queue* queue, const void* item, uint32_t timeout) {
    if (!queue || !item) {
        return QUEUE_ERROR;
    }

    disable_interrupts();

    if (!queue_is_full(queue)) {
        // Adjust head pointer
        queue->head = (queue->head - 1 + queue->queue_length) % queue->queue_length;
        copy_to_queue(queue, item, queue->head);
        queue->items_count++;

        // Wake up one waiting receiver if any
        if (queue->waiting_count_recv > 0) {
            uint32_t task_to_wake = queue->waiting_tasks_recv[0];
            queue->waiting_count_recv--;
            resume_task(task_to_wake);
        }

        notify_queue_event(queue, QUEUE_NOTIFY_ON_SEND);

        enable_interrupts();
        return QUEUE_OK;
    }

    if (timeout == 0) {
        queue->overflow_count++;
        enable_interrupts();
        return QUEUE_FULL;
    }

    // Add current task to sending waiting list
    uint32_t current_task = get_current_task_id();
    queue->waiting_tasks_send[queue->waiting_count_send++] = current_task;

    enable_interrupts();
    block_task(timeout);

    return queue_is_full(queue) ? QUEUE_TIMEOUT : QUEUE_OK;
}

QueueStatus queue_overwrite(Queue* queue, const void* item) {
    if (!queue || !item) {
        return QUEUE_ERROR;
    }

    disable_interrupts();

    if (queue_is_full(queue)) {
        // Overwrite oldest item
        copy_to_queue(queue, item, queue->head);
        queue->head = (queue->head + 1) % queue->queue_length;
        queue->tail = (queue->tail + 1) % queue->queue_length;
        queue->overflow_count++;
    } else {
        copy_to_queue(queue, item, queue->tail);
        queue->tail = (queue->tail + 1) % queue->queue_length;
        queue->items_count++;
    }

    notify_queue_event(queue, QUEUE_NOTIFY_ON_SEND);

    enable_interrupts();
    return QUEUE_OK;
}

void queue_set_notification(Queue* queue, QueueCallback callback, void* context, QueueNotifyType type) {
    if (queue) {
        disable_interrupts();
        queue->notify_callback = callback;
        queue->notify_context = context;
        queue->notify_type = type;
        enable_interrupts();
    }
}

uint32_t queue_get_space_available(const Queue* queue) {
    return queue ? (queue->queue_length - queue->items_count) : 0;
}

// Example usage:
/*
void example_queue_usage(void) {
    Queue* my_queue;
    QueueStatus status;

    // Create a queue for 10 integers
    status = queue_create(&my_queue, sizeof(int), 10);
    if (status != QUEUE_OK) {
        // Handle error
        return;
    }

    // Send data
    int send_data = 123;
    status = queue_send(my_queue, &send_data, 100); // timeout 100ms

    // Receive data
    int receive_data;
    status = queue_receive(my_queue, &receive_data, 100); // timeout 100ms

    // Notification example
    void queue_callback(void* queue, void* context) {
        // Handle queue event
    }

    queue_set_notification(my_queue, queue_callback, NULL, QUEUE_NOTIFY_ON_FULL);

    // Cleanup
    queue_delete(my_queue);
}
*/
