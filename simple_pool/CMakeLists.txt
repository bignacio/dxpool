cmake_minimum_required(VERSION 3.21)
project(dyn_mem_pool VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_C_CLANG_TIDY clang-tidy -checks=-*,bugprone-*,performance-*,clang-*,llvm-*,llvm-libc-*,misc-*,modernize-*,readability-*,cert-*,-llvm-header-guard -header-filter=.* --warnings-as-errors=* -p=build)

add_compile_options(
    -Wall
    -Wpedantic
    -Werror
    -Wextra
    -Wconversion
    -Wsign-conversion
    -Walloca
    -Wshadow
    -Wfloat-equal
    -Wswitch-enum
    -Wcast-qual
    -Wimplicit-fallthrough
    -Wundef
    -Wfloat-equal
    -march=native
    -fPIE
    -fno-omit-frame-pointer
)

add_executable(dyn_mem_pool_test dyn_mem_pool_test.c)
target_link_libraries(dyn_mem_pool_test pthread)
add_executable(dyn_mem_pool_fuzz dyn_mem_pool_fuzz.c)
target_link_libraries(dyn_mem_pool_fuzz pthread)

set_target_properties(dyn_mem_pool_test PROPERTIES CMAKE_C_CLANG_TIDY clang-tidy -checks=-*,bugprone-*,performance-*,clang-*,llvm-*,llvm-libc-*,misc-*,modernize-*,readability-*,cert-*,-llvm-header-guard -header-filter=.* --warnings-as-errors=* -p=build)

add_executable(dyn_mem_pool_fuzz_addr dyn_mem_pool_fuzz.c)
target_compile_options(dyn_mem_pool_fuzz_addr PRIVATE -fsanitize=address)
target_link_options(dyn_mem_pool_fuzz_addr PRIVATE -fsanitize=address)

if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    add_executable(dyn_mem_pool_fuzz_mem dyn_mem_pool_fuzz.c)
    target_compile_options(dyn_mem_pool_fuzz_mem PRIVATE -fsanitize=memory -fsanitize-memory-track-origins=2)
    target_link_options(dyn_mem_pool_fuzz_mem PRIVATE -fsanitize=memory -fsanitize-memory-track-origins=2)
endif()
