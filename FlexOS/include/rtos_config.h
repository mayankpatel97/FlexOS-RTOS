/* rtos_config.h */
#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#define MAX_TASKS           32
#define STACK_SIZE          1024
#define HEAP_SIZE          (32*1024)  // 32KB heap
#define MAX_QUEUES         16
#define MAX_SEMAPHORES     16
#define MAX_MUTEXES        16
#define LOWEST_PRIORITY    0
#define HIGHEST_PRIORITY   7
#define TICKS_PER_SECOND   1000

typedef void (*task_function_t)(void*);

#endif /* RTOS_CONFIG_H */

/* rtos_types.h */
#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H
