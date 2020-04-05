# vulkan support was first introduced in
# https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
#
# if vulkan still isn't found, try updating your
# ${CMAKE_ROOT}/Modules/FindVulkan.cmake
cmake_minimum_required(VERSION 3.7)
project(Tac CXX)


# group lib, dll, exe
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(src)
