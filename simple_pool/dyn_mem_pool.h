#ifndef DYN_MEM_POOL_H
#define DYN_MEM_POOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>

enum {
    NODE_ALIGNMENT = 16,
    CACHE_LINE_SIZE = 64,
};


struct MemNode {
    struct MemNode* next;
    void *data;
    struct MemPool* pool;
} ;


struct MemPool {
    struct MemNode* head;
    size_t mem_size;
#ifdef TRACK_POOL_USAGE
    uint32_t num_allocs;
    uint32_t num_available;
#endif
};



// MemNode operations

void track_pool_usage_allocs(__attribute__((unused)) struct MemPool* pool) {
#ifdef TRACK_POOL_USAGE
    __atomic_add_fetch(&pool->num_allocs, 1, __ATOMIC_RELEASE);
#endif
}

void track_pool_usage_returned(__attribute__((unused)) struct MemPool* pool) {
#ifdef TRACK_POOL_USAGE
    __atomic_add_fetch(&pool->num_available, 1, __ATOMIC_RELEASE);
#endif
}

void track_pool_usage_memnode_unavailable(__attribute__((unused)) struct MemPool* pool) {
#ifdef TRACK_POOL_USAGE
    __atomic_sub_fetch(&pool->num_available, 1, __ATOMIC_RELEASE);
#endif
}

static inline struct MemNode* get_memnode_in_data(void* data) {
    ptrdiff_t *node_mem_val = ((ptrdiff_t *)data) - 1;
    struct MemNode *node = (struct MemNode *)(node_mem_val[0]); // NOLINT(performance-no-int-to-ptr)
    return node;
}

__attribute__((malloc,warn_unused_result))
struct MemNode* alloc_poolable_mem(struct MemPool* pool, size_t size) {
    struct MemNode* node = malloc(sizeof(struct MemNode));
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
    node->pool = pool;
    return node;
}

void free_poolable_mem(struct MemNode* node) {
    // we need to restore all the allocated memory
    ptrdiff_t* ptr_data = node->data;
    void* node_data = ptr_data-1;

    free(node_data);
    free(node);
}

// MemPool operations
/**
 * @brief Creates a new memory pool for a given memory size.
 * The pool will return an memory block of the indicated size
 * Changing the size of the memory block after creation leads to undefined behaviour.
 *
 * @param size of each memory block
 * @return a new memory pool or NULL if the memory for it cannot be allocated
 */
__attribute__((malloc,warn_unused_result))
struct MemPool* alloc_mem_pool(size_t size) {
    struct MemPool* pool = malloc(sizeof(struct MemPool));
    if(pool == NULL) {
        return NULL;
    }
    memset((void*)pool, 0, sizeof(struct MemPool));
    pool->mem_size = size;

    return pool;
}

/**
 * @brief Free the memory used by all items in the pool and empties the pool.
 * This function is not thread safe and should be only invoked when the pool is destroyed
 */
void pool_mem_free_all(struct MemPool* pool) {
    struct MemNode* node = pool->head;
    while(node != NULL) {
        pool->head = node->next;
        free_poolable_mem(node);
        track_pool_usage_memnode_unavailable(pool);
        node = pool->head;
    }

}

void free_mem_pool(struct MemPool* pool) {
    pool_mem_free_all(pool);
    free(pool);
}

// MemPool acquire and return operations
__attribute__((warn_unused_result))
void* pool_mem_try_alloc_data(struct MemPool* pool)  {
    struct MemNode* node = alloc_poolable_mem(pool, pool->mem_size);
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
__attribute__((warn_unused_result))
void* pool_mem_acquire(struct MemPool* pool) {
    while (true) {
        struct MemNode* previous_head = __atomic_load_n(&pool->head, __ATOMIC_ACQUIRE);
        if (previous_head == NULL) {
            return pool_mem_try_alloc_data(pool);
        }

        struct MemNode* new_head = __atomic_load_n(&previous_head->next,__ATOMIC_ACQUIRE);
        if (__atomic_compare_exchange_n(&pool->head, &previous_head, new_head, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            track_pool_usage_memnode_unavailable(pool);
            return previous_head->data;
        }

    }
}

void pool_mem_return(void* data) {
    struct MemNode* new_head = get_memnode_in_data(data);
    struct MemPool* pool = new_head->pool;

    while (true) {
        struct MemNode* previous_head = __atomic_load_n(&pool->head, __ATOMIC_ACQUIRE);
        __atomic_store_n(&new_head->next, previous_head, __ATOMIC_RELEASE);
        if (__atomic_compare_exchange_n(&pool->head, &previous_head, new_head, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            track_pool_usage_returned(pool);
            return;
        }
    }
}



#endif //DYN_MEM_POOL_H
