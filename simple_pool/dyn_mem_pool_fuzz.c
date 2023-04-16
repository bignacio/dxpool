#include "dyn_mem_pool.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __linux__
#include <sched.h>
#include <sys/resource.h>
#endif

void print_mem_usage(void) {
#ifdef __linux__
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    printf("Max used RSS memory: %ld k\n", r_usage.ru_maxrss);
#endif
}

void fuzz_alloc_free_poolable_mem(void) {
    const int run_count = 20000;

    const uint32_t sizes[] = {
        64, 512, 1024, 16384, 65536, 524288, 1048576,
    };

    const int size_count = sizeof(sizes) / sizeof(uint32_t);

    print_mem_usage();
    printf("running alloc and free poolable memory tests. sizes=%d, iterations=%d\n", size_count, run_count);

    struct MemPool pool;
    for (int i = 0; i < run_count; i++) {
        for (int s_index = 0; s_index < size_count; s_index++) {
            // assign size directly for testing purposes only
            pool.mem_size = sizes[s_index];

            struct MemNode *node = alloc_poolable_mem(&pool);
            free_poolable_mem(node);
        }
    }

    print_mem_usage();
}

void fuzz_acquire_release_single_threaded(void) {
    const uint32_t alloc_size = 773;
    struct MemPool *pool = alloc_mem_pool(alloc_size);
    const int mem_count = 113;
    void *acquired_mem[mem_count];

    const int run_count = 97;

    print_mem_usage();
    printf("running acquire and release cycles. iterations=%d, alloc size=%d\n", run_count, alloc_size);

    for (int run = 0; run < run_count; run++) {
        // acquire, release, aquire
        for (int i = 0; i < mem_count; i++) {
            acquired_mem[i] = pool_mem_acquire(pool);
            pool_mem_return(acquired_mem[i]);
            acquired_mem[i] = pool_mem_acquire(pool);
        }

        // release all
        for (int i = 0; i < mem_count; i++) {
            pool_mem_return(acquired_mem[i]);
        }

        // acquire only
        for (int i = 0; i < mem_count; i++) {
            acquired_mem[i] = pool_mem_acquire(pool);
        }

        // release again
        for (int i = 0; i < mem_count; i++) {
            pool_mem_return(acquired_mem[i]);
        }
    }

    free_mem_pool(pool);
    print_mem_usage();
}

void thred_yield(void) {
#ifdef __linux__
    sched_yield();
#endif
}

void *pool_acquire_return_fn(void *arg) {
    const int num_runs = 500;
    const int num_memnodes = 33;
    struct MemPool *pool = (struct MemPool *)arg;

    for (int i = 0; i < num_runs; i++) {
        void *all_data[num_memnodes];
        for (int j = 0; j < num_memnodes; j++) {
            all_data[j] = pool_mem_acquire(pool);
            thred_yield();
        }

        for (int j = 0; j < num_memnodes; j++) {
            pool_mem_return(all_data[j]);
            thred_yield();
        }
    }

    return NULL;
}

void fuzz_acquire_release_multi_threaded(void) {
    const uint32_t num_runners = 32;
    const uint32_t memnode_size = 32;
    pthread_t runners[num_runners];

    print_mem_usage();
    printf("running multithreaded acquire and release cycles. runners=%d, alloc size=%d\n", num_runners, memnode_size);

    struct MemPool *pool = alloc_mem_pool(memnode_size);

    for (uint32_t i = 0; i < num_runners; i++) {
        int created = pthread_create(&runners[i], NULL, pool_acquire_return_fn, pool);
        assert(created == 0);
    }

    for (uint32_t i = 0; i < num_runners; i++) {
        pthread_join(runners[i], NULL);
    }

    struct MemNode *node = pool->head;
    while (node != NULL) {
        int node_count = 0;
        struct MemNode *ptr = pool->head;
        while (ptr != NULL) {
            if (ptr == node) {
                node_count++;
            }
            ptr = ptr->next;
        }
        assert(node_count == 1);
        node = node->next;
    }

    free_mem_pool(pool);
    print_mem_usage();
}

void fuzz_acquire_release_multipool(void) {
    const int size_window = 64;
    const int max_bits = (MULTIPOOL_ENTRY_COUNT + DynPoolMinMultiPoolMemNodeSizeBits) - 1;

    print_mem_usage();
    printf("running multipool acquire and release test. Size window = %d\n", size_window);

    struct MultiPool *multipool = multipool_create();

    for (int bit_count = DynPoolMinMultiPoolMemNodeSizeBits; bit_count < max_bits; bit_count++) {
        for (int window_val = -size_window; window_val < size_window; window_val++) {
            uint32_t size = (uint32_t)((1 << bit_count) + window_val);

            void *data = multipool_mem_acquire(multipool, size);
            pool_mem_return(data);
        }
    }

    multipool_free(multipool);
}

int main(void) {
    printf("--\n");
    fuzz_alloc_free_poolable_mem();

    printf("--\n");
    fuzz_acquire_release_single_threaded();

    printf("--\n");
    fuzz_acquire_release_multi_threaded();

    printf("--\n");
    fuzz_acquire_release_multipool();
}
