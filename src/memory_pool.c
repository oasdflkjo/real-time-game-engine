#include "../include/memory_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Memory block header
typedef struct MemoryBlock {
    struct MemoryBlock* next;
    size_t index;
} MemoryBlock;

// Memory pool structure
struct MemoryPool {
    void* memory;
    size_t block_size;
    size_t block_count;
    size_t used_blocks;
    MemoryBlock* free_list;
};

// Create a new memory pool
MemoryPool* memory_pool_create(size_t block_size, size_t block_count) {
    // Ensure block size is at least as large as a memory block header
    if (block_size < sizeof(MemoryBlock)) {
        block_size = sizeof(MemoryBlock);
    }
    
    // Allocate the pool structure
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) {
        printf("[MemoryPool] ERROR: Failed to allocate pool structure\n");
        return NULL;
    }
    
    // Allocate the memory for all blocks
    size_t total_size = block_size * block_count;
    pool->memory = malloc(total_size);
    if (!pool->memory) {
        printf("[MemoryPool] ERROR: Failed to allocate %zu bytes for pool\n", total_size);
        free(pool);
        return NULL;
    }
    
    // Initialize pool properties
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->used_blocks = 0;
    pool->free_list = NULL;
    
    // Initialize the free list
    memory_pool_reset(pool);
    
    printf("[MemoryPool] Created pool with %zu blocks of %zu bytes each (total: %zu bytes)\n",
           block_count, block_size, total_size);
    
    return pool;
}

// Destroy a memory pool
void memory_pool_destroy(MemoryPool* pool) {
    if (!pool) {
        return;
    }
    
    if (pool->used_blocks > 0) {
        printf("[MemoryPool] WARNING: Destroying pool with %zu blocks still in use\n", 
               pool->used_blocks);
    }
    
    free(pool->memory);
    free(pool);
    
    printf("[MemoryPool] Pool destroyed\n");
}

// Reset the pool (free all allocations)
void memory_pool_reset(MemoryPool* pool) {
    if (!pool) {
        return;
    }
    
    // Initialize all blocks and link them together
    pool->free_list = NULL;
    
    for (int i = pool->block_count - 1; i >= 0; i--) {
        MemoryBlock* block = (MemoryBlock*)((char*)pool->memory + (i * pool->block_size));
        block->next = pool->free_list;
        block->index = i;
        pool->free_list = block;
    }
    
    pool->used_blocks = 0;
    
    printf("[MemoryPool] Pool reset, all blocks freed\n");
}

// Allocate a block from the pool
void* memory_pool_alloc(MemoryPool* pool) {
    if (!pool || !pool->free_list) {
        printf("[MemoryPool] ERROR: Cannot allocate from pool (pool is full or NULL)\n");
        return NULL;
    }
    
    // Get the first free block
    MemoryBlock* block = pool->free_list;
    
    // Update the free list
    pool->free_list = block->next;
    
    // Update statistics
    pool->used_blocks++;
    
    // Clear the memory
    memset(block, 0, pool->block_size);
    
    return block;
}

// Free a block back to the pool
void memory_pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    // Verify the pointer is within the pool's memory range
    if (!memory_pool_contains(pool, ptr)) {
        printf("[MemoryPool] ERROR: Attempted to free pointer not belonging to this pool\n");
        return;
    }
    
    // Cast to a memory block
    MemoryBlock* block = (MemoryBlock*)ptr;
    
    // Add the block to the free list
    block->next = pool->free_list;
    pool->free_list = block;
    
    // Update statistics
    pool->used_blocks--;
}

// Get statistics about the pool
void memory_pool_get_stats(MemoryPool* pool, size_t* total_blocks, size_t* used_blocks) {
    if (!pool) {
        if (total_blocks) *total_blocks = 0;
        if (used_blocks) *used_blocks = 0;
        return;
    }
    
    if (total_blocks) *total_blocks = pool->block_count;
    if (used_blocks) *used_blocks = pool->used_blocks;
}

// Check if a pointer belongs to this pool
bool memory_pool_contains(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) {
        return false;
    }
    
    // Check if the pointer is within the pool's memory range
    return (ptr >= pool->memory && 
            ptr < (void*)((char*)pool->memory + (pool->block_count * pool->block_size)));
} 