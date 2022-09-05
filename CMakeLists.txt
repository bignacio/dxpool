cmake_minimum_required(VERSION 3.21)

project(DXPool VERSION 0.1.0)

# only download dependencies and build all tests if were developing in it
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  include(${CMAKE_CURRENT_SOURCE_DIR}/main.cmake)
else()
  set(DXPOOL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" PARENT_SCOPE)
endif()