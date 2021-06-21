# vulkan first supported in https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
# if vulkan still isn't found, update ${CMAKE_ROOT}/Modules/FindVulkan.cmake
cmake_minimum_required(VERSION 3.7)
project(Tac CXX)

if( FALSE )
  # group lib, dll, exe
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if (MSVC)
    add_compile_options(/W4) # warning level 4
    add_compile_options(/WX) # treat linker warnings as errors
    add_compile_options(/MP) # multi processor compilation
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(src)
add_subdirectory(assets/hlsl)



