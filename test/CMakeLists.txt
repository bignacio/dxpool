message(STATUS "configuring unit tests")

file (GLOB TEST_SOURCES_LIST "*.cpp")

add_executable (dxpool_tests ${TEST_SOURCES_LIST})
target_link_libraries (dxpool_tests PRIVATE Catch2::Catch2WithMain DXPoolStatic)

# When using FetchContent, include(Catch) will fail unless CMAKE_MODULE_PATH is explicitly updated to include the extras directory.
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)
catch_discover_tests(dxpool_tests)
