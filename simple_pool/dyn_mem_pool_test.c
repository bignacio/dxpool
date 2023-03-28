#define TRACK_POOL_USAGE
#include "dyn_mem_pool.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_TEST_NAME printf("-- %s\n", __ASSERT_FUNCTION)
#define ASSERT_MSG(cond, message) assert((cond) && (message))
#define FAIL_TEST(message) assert((message) && false) // NOLINT

void test_alloc_mem_node(void) { // NOLINT(readability-function-cognitive-complexity)
    PRINT_TEST_NAME;
    const size_t mem_size = 471;
    struct MemPool pool;
    struct MemNode *node = alloc_poolable_mem(&pool, mem_size);

    ASSERT_MSG(node->data != NULL, "allocated data should not be null");
    ASSERT_MSG(node->next == NULL, "next node should be null");
    ASSERT_MSG(node->pool == &pool, "owning pool should be set");

    ptrdiff_t *node_mem_val = ((ptrdiff_t *)node->data) - 1;

    struct MemNode *owning_node = (struct MemNode *)(node_mem_val[0]); // NOLINT(performance-no-int-to-ptr)
    ASSERT_MSG(owning_node == node, "node data should keep track of the owning node");
    ASSERT_MSG(owning_node->data == node->data, "node data should be the same");

    free_poolable_mem(node);
}

void test_get_memnode_in_data(void) {
    PRINT_TEST_NAME;
    struct MemPool pool;
    const size_t mem_size = 1024;
    struct MemNode *node = alloc_poolable_mem(&pool, mem_size);

    struct MemNode *node_in_data = get_memnode_in_data(node->data);

    ASSERT_MSG(node == node_in_data, "obtained node should be equal to original");
}

void test_create_pool(void) {
    PRINT_TEST_NAME;
    const size_t alloc_size = 4762;
    struct MemPool *pool = alloc_mem_pool(alloc_size);
    ASSERT_MSG(pool->mem_size == alloc_size, "the new pool should the correct size set");
    ASSERT_MSG(pool->head == NULL, "new pool should have a null head");

    free_mem_pool(pool);
}

void test_acquire_empty_pool(void) {
    PRINT_TEST_NAME;

    const size_t alloc_size = 971;
    struct MemPool *pool = alloc_mem_pool(alloc_size);

    void *data = pool_mem_acquire(pool);

    ASSERT_MSG(data != NULL, "empty pool should create new node with data");

    // manually release alloc memory without returning it to the pool
    // this is just to keep the test simple, in real world usage cases, the allocated data
    // should be returned to the pool and the pool cleared
    struct MemNode *node = get_memnode_in_data(data);
    free_poolable_mem(node);
    free_mem_pool(pool);
}

void test_acquire_and_return_once(void) { // NOLINT(readability-function-cognitive-complexity)
    PRINT_TEST_NAME;

    const size_t alloc_size = 377;
    struct MemPool *pool = alloc_mem_pool(alloc_size);

    void *data = pool_mem_acquire(pool);
    ASSERT_MSG(pool->num_allocs == 1, "allocated memory node should be tracked");
    ASSERT_MSG(pool->num_available == 0, "there should be no memory node available in the pool");

    pool_mem_return(data);
    ASSERT_MSG(pool->num_allocs == 1, "allocated memory node should be tracked");
    ASSERT_MSG(pool->num_available == 1, "there should one node available in the pool");

    pool_mem_free_all(pool);
    ASSERT_MSG(pool->num_allocs == 1, "allocated memory node should be tracked");
    ASSERT_MSG(pool->num_available == 0, "all memory nodes should have been freed");
    ASSERT_MSG(pool->head == NULL, "the pool should be empty");

    free_mem_pool(pool);
}

void test_returned_mem_can_be_reused(void) { // NOLINT(readability-function-cognitive-complexity)
    PRINT_TEST_NAME;

    const size_t alloc_size = 1024;
    struct MemPool *pool = alloc_mem_pool(alloc_size);

    void *first_acquired = pool_mem_acquire(pool);
    pool_mem_return(first_acquired);
    ASSERT_MSG(pool->num_allocs == 1, "allocated memory node should be tracked");
    ASSERT_MSG(pool->num_available == 1, "there should one available memory node in the pool");

    void *second_acquired = pool_mem_acquire(pool);
    ASSERT_MSG(first_acquired == second_acquired, "memory should have been reused");
    ASSERT_MSG(pool->num_available == 0, "there should no available memory node in the pool");

    free_mem_pool(pool);
}

void test_acquire_return_many(void) { // NOLINT(readability-function-cognitive-complexity)
    PRINT_TEST_NAME;
    const size_t alloc_size = 1024;
    struct MemPool *pool = alloc_mem_pool(alloc_size);
    const int mem_count = 12;
    void *acquired_mem[mem_count];

    for (int i = 0; i < mem_count; i++) {
        acquired_mem[i] = pool_mem_acquire(pool);
        pool_mem_return(acquired_mem[i]);
        acquired_mem[i] = pool_mem_acquire(pool);
    }

    // check all memory entries are unique
    // brute force but it's ok for this test
    for (int i = 0; i < mem_count; i++) {
        int found_count = 0;
        for (int j = 0; j < mem_count; j++) {
            if (acquired_mem[i] == acquired_mem[j]) {
                found_count++;
            }
        }
        ASSERT_MSG(found_count == 1, "acquired memory should be unique");
    }
    ASSERT_MSG(pool->num_allocs == mem_count, "allocated memory node should be tracked");
    ASSERT_MSG(pool->num_available == 0, "there should be no available entries in the pool");

    for (int i = 0; i < mem_count; i++) {
        pool_mem_return(acquired_mem[i]);
    }
    ASSERT_MSG(pool->num_available == mem_count, "all allocated memory should have been returned");

    pool_mem_free_all(pool);
    ASSERT_MSG(pool->num_available == 0, "there should be no available entries in the pool after all free");

    free_mem_pool(pool);
}

int main(void) {
    // memory node allocation
    test_alloc_mem_node();
    test_get_memnode_in_data();

    // pool creation and destruction
    test_create_pool();
    test_acquire_empty_pool();
    test_acquire_and_return_once();
    test_returned_mem_can_be_reused();
    test_acquire_return_many();
}
