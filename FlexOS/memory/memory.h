/* memory.h */
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "rtos_config.h"

void memory_init(void);
void* memory_alloc(size_t size);
void memory_free(void* ptr);
size_t memory_get_free_size(void);
void memory_get_stats(size_t* total, size_t* used, size_t* peak);

#endif /* MEMORY_H */
