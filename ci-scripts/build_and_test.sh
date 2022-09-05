#!/bin/bash

rm -rf build

for build_type in Release Debug ; do
    for std in 11 14 17 20 ; do
        echo "Building type '${build_type}' standard ${std}"
        make -B build clean
        cmake -B build \
            -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_CXX_STANDARD=${std} \
            -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_CXX_EXTENSIONS=OFF

        make -j2 -C build
        make -C build test
    done
done