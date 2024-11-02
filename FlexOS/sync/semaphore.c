

/* sync.c */
#include "sync.h"
#include "scheduler.h"

void sem_init(Semaphore* sem, uint32_t initial_count) {
    sem->count = initial_count;
    sem->waiting_list = NULL;
}

bool sem_wait(Semaphore* sem, uint32_t timeout) {
    // Disable interrupts
    disable_interrupts();

    if (sem->count > 0) {
        sem->count--;
        enable_interrupts();
        return true;
    }

    // No resources available, block task
    TCB* current_task = get_current_task();
    current_task->waiting_on = sem;
    current_task->blocked_timeout = timeout;

    // Add to waiting list
    current_task->next = sem->waiting_list;
    sem->waiting_list = current_task;

    block_task(timeout);
    enable_interrupts();

    // When task resumes, check if it was due to timeout
    return current_task->waiting_on == NULL;
}

void sem_signal(Semaphore* sem) {
    disable_interrupts();

    sem->count++;

    // If tasks are waiting, wake up the first one
    if (sem->waiting_list != NULL) {
        TCB* task = sem->waiting_list;
        sem->waiting_list = task->next;
        task->next = NULL;
        task->waiting_on = NULL;
        task->state = TASK_READY;
        sem->count--;
    }

    enable_interrupts();
}

void mutex_init(Mutex* mutex) {
    mutex->owner = NULL;
    mutex->count = 0;
    mutex->waiting_list = NULL;
}

bool mutex_lock(Mutex* mutex, uint32_t timeout) {
    disable_interrupts();

    TCB* current_task = get_current_task();

    // Check if current task already owns the mutex
    if (mutex->owner == current_task) {
        mutex->count++;
        enable_interrupts();
        return true;
    }

    // If mutex is free, take it
    if (mutex->owner == NULL) {
        mutex->owner = current_task;
        mutex->count = 1;
        enable_interrupts();
        return true;
    }

    // Mutex is taken, block task
    current_task->waiting_on = mutex;
    current_task->blocked_timeout = timeout;

    // Add to waiting list
    current_task->next = mutex->waiting_list;
    mutex->waiting_list = current_task;

    block_task(timeout);
    enable_interrupts();

    return current_task->waiting_on == NULL;
}

void mutex_unlock(Mutex* mutex) {
    disable_interrupts();

    TCB* current_task = get_current_task();

    // Check if current task owns the mutex
    if (mutex->owner != current_task) {
        enable_interrupts();
        return;
    }

    // Decrease count for nested locks
    if (--mutex->count == 0) {
        // If tasks are waiting, give mutex to first waiting task
        if (mutex->waiting_list != NULL) {
            TCB* task = mutex->waiting_list;
            mutex->waiting_list = task->next;
            task->next = NULL;
            task->waiting_on = NULL;
            task->state = TASK_READY;
            mutex->owner = task;
            mutex->count = 1;
        } else {
            mutex->owner = NULL;
        }
    }

    enable_interrupts();
}
