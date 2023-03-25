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

    for (int i = 0; i < run_count; i++) {
        for (int s_index = 0; s_index < size_count; s_index++) {
            size_t size = sizes[s_index];
            MemNode *node = alloc_poolable_mem(size);
            free_poolable_mem(node);
        }
    }

    print_mem_usage();
}

int main(void) {
    fuzz_alloc_free_poolable_mem();
}
