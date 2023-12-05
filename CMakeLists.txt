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
  cmake_print_variables("cmake binary dir: " ${CMAKE_BINARY_DIR})
endif()


if (MSVC)

    set( CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
    add_compile_options( /ZI  # program database for edit and continue
                         /W4  # warning level 4
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

    # vma library on macos
    add_compile_options( #-Wno-nullability-completeness 
                         -Wno-nullability-extension
                         #-Wno-nonnull
                         )

endif()

# allow including files from #include "src/..."
include_directories( ${CMAKE_SOURCE_DIR} )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 23) # modules, ie import std.filesystem
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if( MSVC)
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
endif()





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

if(FALSE)
  set( MODULE_PROP "${CMAKE_SOURCE_DIR}/build_std_modules.props" )
  if(EXISTS ${MODULE_PROP} )
    message( "props file path: ${MODULE_PROP}" )
    SET_PROPERTY( GLOBAL PROPERTY VS_USER_PROPS ${MODULE_PROP} )
  else()
    message( FATAL_ERROR "fuck")
  endif()
endif()

# This function 

# For each project, enable it's Visual Studio Project
#   --> Property Pages
#   --> Configuration Properties 
#   --> C/C++
#   --> Language
#   --> Build ISO C++ 23 Standard Library Modules
if(TRUE)
  set( MODULE_PROP "${CMAKE_SOURCE_DIR}/build_std_modules.props" )
  if(EXISTS ${MODULE_PROP} )
    message( "props file path: ${MODULE_PROP}" )


    # This function recursively iterates all folders, and appends each target name into
    # the output variable (_get_all_cmake_targets)
    function (_get_all_cmake_targets out_var current_dir)
        get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
        get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

        foreach(subdir ${subdirs})
            _get_all_cmake_targets(subdir_targets ${subdir})
            list(APPEND targets ${subdir_targets})
        endforeach()

        set(${out_var} ${targets} PARENT_SCOPE)
    endfunction()

    _get_all_cmake_targets(all_targets ${CMAKE_CURRENT_LIST_DIR})
    foreach(target ${all_targets})
      
      if( FALSE )
        message ("cur target: ${target}")
      endif()

      SET_PROPERTY( TARGET ${target} PROPERTY VS_USER_PROPS ${MODULE_PROP} )

    endforeach()
  else()
    message( FATAL_ERROR "fuck")
  endif()
endif()



