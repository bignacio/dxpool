#!/bin/bash

rm -rf build

for build_type in Release Debug ; do
    make -B build clean
    cmake -B build -DCMAKE_BUILD_TYPE=${build_type}

    make -j2 -C build
    build/dyn_mem_pool_test
    build/dyn_mem_pool_fuzz
    build/dyn_mem_pool_fuzz_addr

    if [ "$CC" = "clang" ]; then
        build/dyn_mem_pool_fuzz_mem
    fi
done