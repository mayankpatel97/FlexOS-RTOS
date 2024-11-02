
/* rtos_types.h */
#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "rtos_config.h"

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} TaskState;

typedef struct TCB {
    uint32_t* stack_ptr;           // Current stack pointer
    uint32_t stack[STACK_SIZE];    // Task stack
    TaskState state;               // Current state
    uint8_t priority;              // Task priority
    uint32_t time_slice;           // Time slice for round-robin
    uint32_t blocked_timeout;      // Timeout for blocked state
    task_function_t task_function; // Task function pointer
    void* arg;                     // Task argument
    const char* name;              // Task name
    void* waiting_on;              // Pointer to object task is waiting on
    struct TCB* next;             // Next TCB in list (for waiting lists)
} TCB;

typedef struct {
    TCB tasks[MAX_TASKS];          // Array of TCBs
    uint32_t current_task;         // Index of current task
    uint32_t next_task;           // Index of next task to run
    uint32_t task_count;          // Number of tasks
    bool scheduler_started;        // Scheduler state
    uint32_t system_ticks;        // System tick counter
} Scheduler;

#endif /* RTOS_TYPES_H */
Las
