if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "configuring examples")

    add_executable (workerpool "WorkerPoolApp.cpp")
    target_include_directories(workerpool PRIVATE "../src")

    add_executable (datapool "DataPoolApp.cpp")
    target_include_directories(datapool PRIVATE "../src")
else()
    message(STATUS "Skipping building examples for unsupported system ${CMAKE_SYSTEM_NAME}")
endif()