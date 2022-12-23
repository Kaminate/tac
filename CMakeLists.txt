include( CMakePrintHelpers )

# vulkan first supported in https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
# if vulkan still isn't found, update ${CMAKE_ROOT}/Modules/FindVulkan.cmake
cmake_minimum_required(VERSION 3.25.1)
cmake_policy(SET CMP0072 NEW) # prefer latest opengl
cmake_policy(SET CMP0076 NEW) # convert relative paths to absolute paths in target_sources
project(Tac CXX)

if( TRUE )
  # group lib, dll, exe
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

cmake_print_variables("cmake binary dir: " ${CMAKE_BINARY_DIR})

if (MSVC)
    add_compile_options( /W4  # warning level 4
                         /WX  # treat linker warnings as errors
                         /MP  # multi processor compilation
                         /experimental:module) # modules???
else() # gcc, clang
    add_compile_options( -Wall
                         -Wextra
                         -Werror
                         -Wpedantic )

    # suppress annoying warnings
    add_compile_options( -Wno-comment
                         -Wno-missing-field-initializers
                         -Wno-reorder
                         -Wno-switch
                         -Wno-unused-but-set-variable
                         -Wno-unused-parameter
                         -Wno-unused-value
                         -Wno-unused-variable )

endif()

# allow including files from #include "src/..."
include_directories( ${CMAKE_SOURCE_DIR} )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#===----------------===#
# Clone git submodules #

find_package(Git REQUIRED)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT )
  if( NOT GIT_SUBMOD_RESULT EQUAL "0" )
    message(FATAL_ERROR "failed to update submodules, because ${GIT_SUBMOD_RESULT}")
  endif()
endif()

# Clone git submodules #
#===----------------===#

add_subdirectory(src)
add_subdirectory(assets/hlsl)


