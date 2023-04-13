#ifndef DYN_MEM_POOL_H
#define DYN_MEM_POOL_H
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

struct MemNode {
    struct MemNode* next;
    void *data;
    struct MemPool* pool;
} ;


struct MemPool {
    struct MemNode* head;
    uint32_t mem_size;
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
    assert(data != NULL);

    ptrdiff_t *node_mem_val = ((ptrdiff_t *)data) - 1;
    struct MemNode *node = (struct MemNode *)(node_mem_val[0]); // NOLINT(performance-no-int-to-ptr)
    return node;
}

__attribute__((malloc,warn_unused_result))
struct MemNode* alloc_poolable_mem(struct MemPool* pool, uint32_t size) {
    assert(pool != NULL);
    assert(size != 0);

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
    assert(node != NULL);

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
struct MemPool* alloc_mem_pool(uint32_t size) {
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
    assert(pool != NULL);

    struct MemNode* node = pool->head;
    while(node != NULL) {
        pool->head = node->next;
        free_poolable_mem(node);
        track_pool_usage_memnode_unavailable(pool);
        node = pool->head;
    }
}

void free_mem_pool(struct MemPool* pool) {
    assert(pool != NULL);

    pool_mem_free_all(pool);
    free(pool);
}

// MemPool acquire and return operations
__attribute__((warn_unused_result))
void* pool_mem_try_alloc_data(struct MemPool* pool)  {
    assert(pool != NULL);

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
    assert(pool != NULL);

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
    assert(data != NULL);

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

/*
    Multi pool support allows allocation of arbitrary memory sizes, distributing them across multiple pools
    at the expense of extra allocated memory if the requested size doesn't match the defined pool memory size
*/


// Number of bits indicating the minimum amount of bytes allocated in a multi pool setup
static const int DynPoolMinMultiPoolMemNodeSizeBits = 9;


enum {
    // 23 bits = 4Mb max with the first 9 mapping to the first pool
    MULTIPOOL_ENTRY_COUNT = 14
};

struct MultiPool {
    struct MemPool* pools[MULTIPOOL_ENTRY_COUNT];
};


/**
 * @brief Allocates a new multi pool with a maximum of MULTIPOOL_ENTRY_COUNT entries
 *
 * @return the created multi pool
 */
__attribute__((warn_unused_result))
struct MultiPool* multipool_create(void) {
    struct MultiPool* multipool = malloc(sizeof(struct MultiPool));

    if(multipool != NULL) {
        for(uint32_t i = 0 ; i < MULTIPOOL_ENTRY_COUNT ; i++) {
            uint32_t size = 1 << ((uint32_t)DynPoolMinMultiPoolMemNodeSizeBits+i);
            struct MemPool* pool = alloc_mem_pool(size);
            multipool->pools[i] = pool;
        }
    }

    return multipool;
}

/**
 * @brief Releases all memory owned by a multi pool, including individual pool items
 *
 * @param multipool
 */
void multipool_free(struct MultiPool* multipool) {
    assert(multipool != NULL);

    for(int i = 0 ; i < MULTIPOOL_ENTRY_COUNT ; i++) {
        free_mem_pool(multipool->pools[i]);
    }
    free(multipool);
}

/**
 * @brief Finds the index of the pool where a memory node of size `size` can be allocated
 *
 * @param size the size of the memory node to be allocated. If size is zero, the return value is undefined
 * @return uint32_t index of pool to hold the memory node
 */
uint32_t find_multipool_index_for_size(uint32_t size) {
    static const uint32_t int32bitcount = sizeof(uint32_t) * CHAR_BIT;
    uint32_t size_value = (size - 1) >>DynPoolMinMultiPoolMemNodeSizeBits;
    return (int32bitcount - (uint32_t)__builtin_clz(size_value));
}

/**
 * @brief Acquire memory in a multipool setup. Acquired memory should be returned calling pool_mem_return
 *
 * @param multipool the multipool to be used
 * @param size size of the memory allocated. It will be rounded up to the next power of 2 bit
 * @return void* allocated memory or NULL if allocation fail
 */
void* multipool_mem_acquire(struct MultiPool* multipool, uint32_t size) {
    assert(multipool != NULL);

    uint32_t index = find_multipool_index_for_size(size);

    assert(index < MULTIPOOL_ENTRY_COUNT);
    if (__builtin_expect(index >= MULTIPOOL_ENTRY_COUNT, 0)) {
        return NULL;
    }
    struct MemPool* pool = multipool->pools[index];

    return pool_mem_acquire(pool);
}

/**
 * @brief Global multipool support
 *
 */
struct MultiPool* _global_multipool = NULL; //NOLINT(bugprone-reserved-identifier, cppcoreguidelines-avoid-non-const-global-variables)

/**
 * @brief Create the global multipool
 *
 */
void global_multipool_create(void) {
    _global_multipool = multipool_create();
}

/**
 * @brief Acquires a block of memory from the global multipool. Acquired memory should be returned calling pool_mem_return
 *
 * @param size of block to be acquired
 * @return void* pointer to acquired block
 */
void* global_multipool_mem_acquire(uint32_t size) {
    assert(_global_multipool != NULL);
    return multipool_mem_acquire(_global_multipool, size);
}

/**
 * @brief Destroys the global multipool releasing all memory stored in the pool
 *
 */
void global_multipool_free(void) {
    multipool_free(_global_multipool);
}

#endif //DYN_MEM_POOL_H
