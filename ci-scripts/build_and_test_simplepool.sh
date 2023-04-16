#!/bin/bash

set -e
cd simple_pool

rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

make -j2 -C build
build/dyn_mem_pool_test
build/dyn_mem_pool_fuzz
build/dyn_mem_pool_fuzz_addr

if [ "$CC" = "clang" ]; then
    build/dyn_mem_pool_fuzz_mem
fi
