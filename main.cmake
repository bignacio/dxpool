if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Building for C++ standard ${CMAKE_CXX_STANDARD}")

include(FetchContent)
include(ExternalProject)

# dependencies
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.1.1
)
FetchContent_MakeAvailable(Catch2)

# clang tidy setup
set(CMAKE_CXX_CLANG_TIDY clang-tidy --config-file=${CMAKE_CURRENT_SOURCE_DIR}/clang-tidy.config -p=build)

# source files setup
set (DXPool_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file (GLOB DXPool_SOURCES_LIST "${DXPool_SOURCE_DIR}/*.cpp")

# setup build target and sources
add_library(DXPoolObjLib OBJECT ${DXPool_SOURCES_LIST})
set_property(TARGET DXPoolObjLib PROPERTY POSITION_INDEPENDENT_CODE 1)

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
)

add_library (DXPoolStatic STATIC "")

target_sources (DXPoolStatic PRIVATE $<TARGET_OBJECTS:DXPoolObjLib>)

set_target_properties (DXPoolStatic PROPERTIES LINKER_LANGUAGE CXX DEBUG_POSTFIX "-d")

enable_testing()

add_subdirectory(fuzz)
add_subdirectory (test)
add_subdirectory(benchmark)
add_subdirectory(examples)