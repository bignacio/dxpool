#include "dyn_mem_pool.h"
#include <stdio.h>

#ifdef __linux__
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

    const size_t sizes[] = {
        64, 512, 1024, 16384, 65536, 524288, 1048576,
    };

    const int size_count = sizeof(sizes) / sizeof(size_t);

    print_mem_usage();
    printf("running alloc and free poolable memory tests. sizes=%d, iterations=%d\n", size_count, run_count);

    struct MemPool pool;
    for (int i = 0; i < run_count; i++) {
        for (int s_index = 0; s_index < size_count; s_index++) {
            size_t size = sizes[s_index];
            struct MemNode *node = alloc_poolable_mem(&pool, size);
            free_poolable_mem(node);
        }
    }

    print_mem_usage();
}

void fuzz_acquire_release_single_thread(void) {
    const size_t alloc_size = 773;
    struct MemPool *pool = alloc_mem_pool(alloc_size);
    const int mem_count = 113;
    void *acquired_mem[mem_count];

    const int run_count = 97;

    print_mem_usage();
    printf("running acquire and release cycles. iterations=%d, alloc size=%zu\n", run_count, alloc_size);

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

int main(void) {
    printf("--\n");
    fuzz_alloc_free_poolable_mem();
    printf("--\n");
    fuzz_acquire_release_single_thread();
}
