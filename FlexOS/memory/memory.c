
/* memory.c */
#include "memory.h"

// Memory block structure
typedef struct MemoryBlock {
    size_t size;                 // Size of the block
    bool is_free;               // Is the block free?
    struct MemoryBlock* next;   // Next block in the list
} MemoryBlock;

// Memory pool
static uint8_t heap[HEAP_SIZE];
static MemoryBlock* first_block;
static size_t peak_usage = 0;
static size_t current_usage = 0;

void memory_init(void) {
    // Initialize first block
    first_block = (MemoryBlock*)heap;
    first_block->size = HEAP_SIZE - sizeof(MemoryBlock);
    first_block->is_free = true;
    first_block->next = NULL;
}

void* memory_alloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;

    MemoryBlock* current = first_block;
    MemoryBlock* best_fit = NULL;
    size_t smallest_suitable_size = HEAP_SIZE;

    // Find best fit block
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size < smallest_suitable_size) {
                best_fit = current;
                smallest_suitable_size = current->size;
            }
        }
        current = current->next;
    }

    if (best_fit == NULL) {
        return NULL;  // No suitable block found
    }

    // Split block if it's too large
    if (best_fit->size >= size + sizeof(MemoryBlock) + 8) {
        MemoryBlock* new_block = (MemoryBlock*)((uint8_t*)best_fit + sizeof(MemoryBlock) + size);
        new_block->size = best_fit->size - size - sizeof(MemoryBlock);
        new_block->is_free = true;
        new_block->next = best_fit->next;

        best_fit->size = size;
        best_fit->next = new_block;
    }

    best_fit->is_free = false;
    current_usage += best_fit->size;
    if (current_usage > peak_usage) {
        peak_usage = current_usage;
    }

    return (void*)((uint8_t*)best_fit + sizeof(MemoryBlock));
}

void memory_free(void* ptr) {
    if (ptr == NULL) return;

    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    block->is_free = true;
    current_usage -= block->size;

    // Coalesce with next block if it's free
    if (block->next != NULL && block->next->is_free) {
        block->size += block->next->size + sizeof(MemoryBlock);
        block->next = block->next->next;
    }

    // Find previous block to coalesce
    MemoryBlock* prev = first_block;
    while (prev != NULL && prev->next != block) {
        prev = prev->next;
    }

    if (prev != NULL && prev->is_free) {
        prev->size += block->size + sizeof(MemoryBlock);
        prev->next = block->next;
    }
}

size_t memory_get_free_size(void) {
    size_t free_size = 0;
    MemoryBlock* current = first_block;

    while (current != NULL) {
        if (current->is_free) {
            free_size += current->size;
        }
        current = current->next;
    }

    return free_size;
}

void memory_get_stats(size_t* total, size_t* used, size_t* peak) {
    if (total) *total = HEAP_SIZE;
    if (used) *used = current_usage;
    if (peak) *peak = peak_usage;
}
