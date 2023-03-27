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
} MemNode;

typedef struct {
    MemNode* head;
    size_t mem_size;
#ifdef TRACK_POOL_USAGE
    uint32_t num_allocs;
    uint32_t num_available;
#endif
} MemPool;

// MemNode operations

void track_pool_usage_allocs(__attribute__((unused)) MemPool* pool) {
#ifdef TRACK_POOL_USAGE
    __atomic_add_fetch(&pool->num_allocs, 1, __ATOMIC_RELEASE);
#endif
}

__attribute__((nonnull (1)))
static inline MemNode* get_memnode_in_data(void* data) {
    ptrdiff_t *node_mem_val = ((ptrdiff_t *)data) - 1;
    MemNode *node = (MemNode *)(node_mem_val[0]); // NOLINT(performance-no-int-to-ptr)
    return node;
}

__attribute__((malloc))
MemNode* alloc_poolable_mem(size_t size) {
    MemNode* node = malloc(sizeof(MemNode));
    if (node == NULL) {
        return NULL;
    }
    node->next = NULL;
    ptrdiff_t* ptr_data = malloc(size + sizeof(ptrdiff_t));

    if(ptr_data == NULL) {
        free(node);
        return NULL;
    }

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
/**
 * @brief Creates a new memory pool for a given memory size.
 * The pool will return an memory block of the indicated size
 * Changing the size of the memory block after creation leads to undefined behaviour.
 *
 * @param size of each memory block
 */
__attribute__((malloc))
MemPool* alloc_mem_pool(size_t size) {
    MemPool* pool = malloc(sizeof(MemPool));
    if(pool == NULL) {
        return NULL;
    }
    memset((void*)pool, 0, sizeof(MemPool));
    pool->mem_size = size;

    return pool;
}

/**
 * @brief Free the memory used by all items in the pool and empties the pool.
 * This function is not thread safe and should be only invoked when the pool is destroyed
 */
__attribute__((nonnull (1)))
void pool_mem_free_all(MemPool* pool) {
    (void)pool;
}

__attribute__((nonnull (1)))
void free_mem_pool(MemPool* pool) {
    pool_mem_free_all(pool);
    free(pool);
}

// MemPool acquire and return operations

__attribute__((nonnull (1)))
void* pool_mem_try_alloc_data(MemPool* pool) {
    MemNode* node = alloc_poolable_mem(pool->mem_size);
    if(node == NULL) {
        return NULL;
    }

    track_pool_usage_allocs(pool);
    return node->data;
}

/**
 * @brief Acquire a block of memory from the pool. If the pool is empty, new memory will be allocated
 *
 * @param pool the memory pool where objects will be tracked
 * @return void* pointer to allocated memory or NULL if memory cannot be allocated
 */
__attribute__((nonnull (1)))
void* pool_mem_acquire(MemPool* pool) {
    while (true) {
        MemNode* head = __atomic_load_n(&pool->head, __ATOMIC_ACQUIRE);
        if (head == NULL) {
            return pool_mem_try_alloc_data(pool);
        }

        return NULL;
    }
}

__attribute__((nonnull (1,2)))
void pool_mem_return(MemPool* pool, void* data) {
    (void)pool;
    (void)data;
}



#endif //DYN_MEM_POOL_H
