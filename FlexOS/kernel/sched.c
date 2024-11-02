#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 32
#define STACK_SIZE 1024
#define LOWEST_PRIORITY 0
#define HIGHEST_PRIORITY 7

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} TaskState;

// Task Control Block (TCB)
typedef struct {
    uint32_t* stack_ptr;           // Current stack pointer
    uint32_t stack[STACK_SIZE];    // Task stack
    TaskState state;               // Current state
    uint8_t priority;              // Task priority (0-7)
    uint32_t time_slice;           // Time slice for round-robin
    uint32_t blocked_timeout;      // Timeout for blocked state
    void (*task_function)(void*);  // Task function pointer
    void* arg;                     // Task argument
    const char* name;              // Task name
} TCB;

// Scheduler data structure
typedef struct {
    TCB tasks[MAX_TASKS];          // Array of TCBs
    uint32_t current_task;         // Index of current task
    uint32_t next_task;           // Index of next task to run
    uint32_t task_count;          // Number of tasks
    bool scheduler_started;        // Scheduler state
    uint32_t system_ticks;        // System tick counter
} Scheduler;

// Global scheduler instance
static Scheduler scheduler;

// Initialize the scheduler
void scheduler_init(void) {
    scheduler.current_task = 0;
    scheduler.next_task = 0;
    scheduler.task_count = 0;
    scheduler.scheduler_started = false;
    scheduler.system_ticks = 0;
}

// Create a new task
int32_t create_task(void (*task_func)(void*), void* arg, uint8_t priority, const char* name) {
    if (scheduler.task_count >= MAX_TASKS) {
        return -1;  // Task limit reached
    }

    uint32_t task_id = scheduler.task_count;
    TCB* task = &scheduler.tasks[task_id];

    // Initialize stack (platform dependent)
    task->stack_ptr = &task->stack[STACK_SIZE - 16]; // Reserve space for context

    // Initialize TCB
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = 0;
    task->blocked_timeout = 0;
    task->task_function = task_func;
    task->arg = arg;
    task->name = name;

    // Initialize stack frame for context switching
    // This is architecture-specific (example for ARM Cortex-M)
    task->stack[STACK_SIZE - 1] = 0x01000000;    // xPSR
    task->stack[STACK_SIZE - 2] = (uint32_t)task_func; // PC
    task->stack[STACK_SIZE - 3] = 0xFFFFFFFD;    // LR
    task->stack[STACK_SIZE - 8] = (uint32_t)arg; // R0 (argument)

    scheduler.task_count++;
    return task_id;
}

// Find highest priority ready task
static uint32_t find_next_task(void) {
    uint32_t highest_priority = LOWEST_PRIORITY;
    uint32_t next_task = scheduler.current_task;

    // Find highest priority ready task
    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        if (scheduler.tasks[i].state == TASK_READY &&
            scheduler.tasks[i].priority > highest_priority) {
            highest_priority = scheduler.tasks[i].priority;
            next_task = i;
        }
    }

    return next_task;
}

// Schedule next task
void schedule(void) {
    if (!scheduler.scheduler_started || scheduler.task_count == 0) {
        return;
    }

    // Update system ticks
    scheduler.system_ticks++;

    // Check blocked tasks for timeout
    for (uint32_t i = 0; i < scheduler.task_count; i++) {
        if (scheduler.tasks[i].state == TASK_BLOCKED &&
            scheduler.tasks[i].blocked_timeout > 0) {
            if (--scheduler.tasks[i].blocked_timeout == 0) {
                scheduler.tasks[i].state = TASK_READY;
            }
        }
    }

    // Find next task to run
    scheduler.next_task = find_next_task();

    // If current task is different from next task, perform context switch
    if (scheduler.current_task != scheduler.next_task) {
        TCB* current = &scheduler.tasks[scheduler.current_task];
        TCB* next = &scheduler.tasks[scheduler.next_task];

        // Save current context and switch to next task
        // This should trigger PendSV interrupt for context switching
        current->state = TASK_READY;
        next->state = TASK_RUNNING;
        scheduler.current_task = scheduler.next_task;

        // Trigger context switch (platform dependent)
        trigger_context_switch();  // This function needs to be implemented
    }
}

// Start the scheduler
void start_scheduler(void) {
    if (scheduler.task_count == 0) {
        return;
    }

    scheduler.scheduler_started = true;

    // Initialize first task
    scheduler.current_task = find_next_task();
    scheduler.tasks[scheduler.current_task].state = TASK_RUNNING;

    // Start system timer (platform dependent)
    init_system_timer();  // This function needs to be implemented

    // Start first task (platform dependent)
    start_first_task();  // This function needs to be implemented
}

// Block current task
void block_task(uint32_t timeout) {
    if (!scheduler.scheduler_started) {
        return;
    }

    TCB* current = &scheduler.tasks[scheduler.current_task];
    current->state = TASK_BLOCKED;
    current->blocked_timeout = timeout;
    schedule();
}

// Resume a blocked task
void resume_task(uint32_t task_id) {
    if (task_id >= scheduler.task_count) {
        return;
    }

    if (scheduler.tasks[task_id].state == TASK_BLOCKED) {
        scheduler.tasks[task_id].state = TASK_READY;
        scheduler.tasks[task_id].blocked_timeout = 0;
    }
}
