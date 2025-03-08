#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stddef.h>
#include <stdbool.h>

// Memory pool handle
typedef struct MemoryPool MemoryPool;

// Create a new memory pool
MemoryPool* memory_pool_create(size_t block_size, size_t block_count);

// Destroy a memory pool
void memory_pool_destroy(MemoryPool* pool);

// Allocate a block from the pool
void* memory_pool_alloc(MemoryPool* pool);

// Free a block back to the pool
void memory_pool_free(MemoryPool* pool, void* ptr);

// Get statistics about the pool
void memory_pool_get_stats(MemoryPool* pool, size_t* total_blocks, size_t* used_blocks);

// Check if a pointer belongs to this pool
bool memory_pool_contains(MemoryPool* pool, void* ptr);

// Reset the pool (free all allocations)
void memory_pool_reset(MemoryPool* pool);

#endif // MEMORY_POOL_H 