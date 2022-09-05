message(STATUS "configuring benchmark")


file (GLOB BENCH_SOURCES_LIST "*.cpp")

add_executable (dxpool_bench ${BENCH_SOURCES_LIST})
target_compile_options(dxpool_bench PRIVATE "-O3")

target_link_libraries (dxpool_bench PRIVATE Catch2::Catch2WithMain DXPoolStatic)

include(Catch)
