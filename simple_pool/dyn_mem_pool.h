#ifndef DYN_MEM_POOL_H
#define DYN_MEM_POOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum {
    NODE_ALIGNMENT = 16,
    CACHE_LINE_SIZE = 64,
};

typedef struct {
    struct MemNode* next;
    void *data;
    size_t size;
} MemNode;

typedef struct {
    MemNode* head;
#ifdef TRACK_POOL_USAGE
    uint32_t num_allocs;
    uint32_t num_available;
#endif
} MemPool;

// MemNode operations
__attribute__((nonnull (1)))
static inline MemNode* get_memnode_in_data(void* data) {
    ptrdiff_t *node_mem_val = ((ptrdiff_t *)data) - 1;
    MemNode *node = (MemNode *)(node_mem_val[0]); // NOLINT(performance-no-int-to-ptr)
    return node;
}

__attribute__((malloc))
MemNode* alloc_poolable_mem(size_t size) {
    MemNode* node = malloc(sizeof(MemNode));
    node->next = NULL;
    node->size = size;
    ptrdiff_t* ptr_data = malloc(size + sizeof(ptrdiff_t));

    // store the address of the owning node in the first sizeof(ptrdiff_t) byes
    ptr_data[0] = (ptrdiff_t) node;
    node->data = ptr_data+1;
    return node;
}

__attribute__((nonnull (1)))
void free_poolable_mem(MemNode* node) {
    // we need to restore all the allocated memory
    ptrdiff_t* ptr_data = node->data;
    void* all_data = ptr_data-1;

    free(all_data);
    free(node);
}

// MemPool operations
__attribute__((malloc))
MemPool* alloc_mem_pool(void) {
    MemPool* pool = malloc(sizeof(MemPool));
    memset((void*)pool, 0, sizeof(MemPool));

    return pool;
}

__attribute__((nonnull (1)))
void free_mem_pool(MemPool* pool) {
    free(pool);
}

// MemPool acquire and return operations


/**
 * @brief Acquire a block of memory from the pool. If the pool is empty, new memory will be allocated
 *
 * @param pool the memory pool where objects will be tracked
 * @param size of the memory block to be allocated
 * @return void* pointer to allocated memory
 */
__attribute__((nonnull (1)))
void* pool_mem_acquire(MemPool* pool, size_t size) {
    (void) size;
    while (true) {
        MemNode* old_head = __atomic_load_n(&pool->head, __ATOMIC_ACQUIRE);
        if (old_head == NULL) {
            return NULL;
        }
        return old_head->data;

    }
}

__attribute__((nonnull (1,2)))
void pool_mem_return(MemPool* pool, void* data) {
    (void)pool;
    (void)data;
}

/**
 * @brief Free the memory used by all items in the pool and empties the pool.
 * This function is not thread safe and should be only invoked when the pool is destroyed
 */
__attribute__((nonnull (1)))
void pool_mem_free_all(MemPool* pool) {
    (void)pool;
}

#endif //DYN_MEM_POOL_H
