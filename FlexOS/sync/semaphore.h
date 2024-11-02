/* sync.h */
#ifndef SYNC_H
#define SYNC_H

#include "rtos_types.h"

typedef struct {
    uint32_t count;
    TCB* waiting_list;
} Semaphore;

typedef struct {
    TCB* owner;
    uint32_t count;
    TCB* waiting_list;
} Mutex;

// Semaphore functions
void sem_init(Semaphore* sem, uint32_t initial_count);
bool sem_wait(Semaphore* sem, uint32_t timeout);
void sem_signal(Semaphore* sem);

// Mutex functions
void mutex_init(Mutex* mutex);
bool mutex_lock(Mutex* mutex, uint32_t timeout);
void mutex_unlock(Mutex* mutex);

#endif /* SYNC_H */
