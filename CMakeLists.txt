# --------------------------------------------------------------------------------------------------

# 3.30 required for c++23 `import std`
cmake_minimum_required( VERSION 3.31.0 )

# --------------------------------------------------------------------------------------------------

cmake_policy( SET CMP0072 NEW ) # prefer latest opengl
cmake_policy( SET CMP0076 NEW ) # convert relative paths to absolute paths in target_sources

# --------------------------------------------------------------------------------------------------

project( Tac CXX )

# --------------------------------------------------------------------------------------------------

# group lib, dll, exe
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
message( VERBOSE "cmake binary dir: ${CMAKE_BINARY_DIR}" )

# --------------------------------------------------------------------------------------------------

if( MSVC )

    set( CMAKE_VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
    add_compile_options( /ZI  # 
                         /W4  # warning level 4
                         /WX  # treat linker warnings as errors
                         /MP  # multi processor compilation
                         )
                         # /experimental:module) # modules???
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

    # vma library on macOS
    add_compile_options( #-Wno-nullability-completeness 
                         -Wno-nullability-extension
                         #-Wno-nonnull
                         )

endif()

# --------------------------------------------------------------------------------------------------

# allow including files from #include "src/..."
#
# Note Prefer the target_include_directories() command to add include directories to individual
# targets and optionally propagate/export them to dependents.
# include_directories( ${CMAKE_SOURCE_DIR} )

# --------------------------------------------------------------------------------------------------

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

# --------------------------------------------------------------------------------------------------

# https://cmake.org/cmake/help/latest/variable/CMAKE_INCLUDE_CURRENT_DIR.html
# Auto adds CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR to the inc path for each dir.
# These do not propagate down to subdirectories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

# --------------------------------------------------------------------------------------------------

# C++20 Modules, and `import std;`
set( CMAKE_CXX_STANDARD 23 )

# --------------------------------------------------------------------------------------------------

# Clone git submodules (does this work?)

find_package( Git REQUIRED )
if( GIT_FOUND )
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT )
  if( NOT GIT_SUBMOD_RESULT EQUAL "0" )
    message( FATAL_ERROR "failed to update submodules, because ${GIT_SUBMOD_RESULT}" )
  endif()
endif()

# --------------------------------------------------------------------------------------------------

add_subdirectory( src )
add_subdirectory( assets )

# --------------------------------------------------------------------------------------------------

# This function recursively iterates all folders, and appends each target name into
# the output variable (_get_all_cmake_targets)
function( _get_all_cmake_targets out_var current_dir )
    get_property( targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS )
    get_property( subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES )

    foreach( subdir ${subdirs} )
        _get_all_cmake_targets( subdir_targets ${subdir} )
        list(APPEND targets ${subdir_targets} )
    endforeach()

    set( ${out_var} ${targets} PARENT_SCOPE )
endfunction()

# --------------------------------------------------------------------------------------------------

# For each project, enable it's Visual Studio Project
#   --> Property Pages
#   --> Configuration Properties 
#   --> C/C++
#   --> Language
#   --> Build ISO C++ 23 Standard Library Modules

if( FALSE )

  set( MODULE_PROP "${CMAKE_SOURCE_DIR}/build_std_modules.props" )
  if( NOT EXISTS ${MODULE_PROP} )
    message( FATAL_ERROR "cannot find std modules .props" )
  endif()

  _get_all_cmake_targets( all_targets ${CMAKE_CURRENT_LIST_DIR} )
  foreach( target ${all_targets} )
    set_property( TARGET ${target} PROPERTY VS_USER_PROPS ${MODULE_PROP} )
  endforeach()
  list( LENGTH all_targets target_count )
  message( VERBOSE "setting all ${target_count} target VS_USER_PROPS to ${MODULE_PROP}" )

endif()


# --------------------------------------------------------------------------------------------------

