add_library( TacDX )
set_target_properties( TacDX PROPERTIES FOLDER "Engine/Desktop App" )
target_include_directories( TacDX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

add_subdirectory( dxc )
add_subdirectory( dxgi )
add_subdirectory( dx12 )
add_subdirectory( pix )
add_subdirectory( hlsl )


# Download pix runtime library from nuget
if( TRUE )
  include( ${CMAKE_SOURCE_DIR}/cmake/pix.cmake )
  add_pix_target()
  target_link_libraries( TacDX PIX )
  add_custom_command( TARGET TacDX
                      PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${PIX_DLL_PATH}
                              $<TARGET_FILE_DIR:TacDX> )
  target_compile_definitions( TacDX PUBLIC TAC_PIX_NUGET )
endif()


# -------------------------------------------------------------------------------------------------

# Not using FetchContent_Declare( DirectX-Headers ... ) because it fucks import std module

# -------------------------------------------------------------------------------------------------

# In a program that is using dxc to compile shaders via #include <dxcapi.h> and
# #pragma comment (lib, "dxcompiler.lib" ), the call to IDxcCompiler::Compile will emit the
# following warning if dxil.dll is not in the same folder as the target executable:
# +---------------------------------------------------------------------------------------+
# | DXIL.dll not found. Resulting DXIL will not be signed for use in release environments |
# +---------------------------------------------------------------------------------------+
# We define the cmake CACHE vars TAC_DXIL_DLL_PATH and TAC_DXCOMPILER_DLL_PATH to copy the dlls
# to the same folder as the target executable
#
# If $ENV{WindowsSdkVerBinPath} is defined, we use that
#
# Cache TAC_DXIL_DLL_PATH/TAC_DXCOMPILER_DLL_PATH to be used by the shader compiler.
# CMake must have been run after vcvars.bat to have 
# (see tac/run/run_vs2022.bat which runs vcvars.bat prior to CMake)
# The reason TAC_DXIL_DLL_PATH is cached is because cmake can be re-run from inside visual studio,
# which doesn't have $ENV{WindowsSdkVerBinPath}.

# Try to define TAC_DXIL_DLL_PATH and TAC_DXCOMPILER_DLL_PATH using the
# WindowsSdkVerBinPath environment variable. WindowsSdkVerBinPath will be set if vcvars has been
# run or if cmake is being run from the Developer Command Prompt for VS
function( _define_dx_path varName filename )

  set( docStr "path to ${filename} for shader compiling/signing" )
  set( ${varName} "" CACHE FILEPATH ${docStr} )
  set( varVal $CACHE{${varName}} )

  # Use cache value if it exists
  if( EXISTS ${varVal} )
    return()
  endif()

  #message(" sdk bin path dir thing:  $ENV{WindowsSdkVerBinPath}x64/${filename}" )
  #list( APPEND bins "D:/Program Files (x86)/DXC/bin/x64/${filename}" )
  #list( APPEND varVals "$ENV{WindowsSdkVerBinPath}x64/${filename}" )

  # This is just where I downloaded the latest release bins of
  # https://github.com/microsoft/DirectXShaderCompiler/
  list( APPEND bins "C:/Program Files (x86)/DXC/bin/" )
  list( APPEND bins "D:/Program Files (x86)/DXC/bin/" )
  if( DEFINED ENV{WindowsSdkVerBinPath} )
    list( APPEND bins $ENV{WindowsSdkVerBinPath} )
  endif()

  foreach( bin ${bins} )
    set( varVal ${bin}x64/${filename} )
    if( EXISTS ${varVal} )
      message( VERBOSE "Setting ${varName} to ${varVal}" )
      set( ${varName} ${varVal} CACHE FILEPATH ${docStr} FORCE )
      return()
    endif()
  endforeach()

  # set( varVal "D:/Program Files (x86)/DXC/bin/x64/${filename}" )
  # if( EXISTS ${varVal} )
  #   set( ${varName} ${varVal} CACHE FILEPATH ${docStr} FORCE )
  #   return()
  # endif()

  # if( DEFINED ENV{WindowsSdkVerBinPath} )
  #   if( EXISTS "$ENV{WindowsSdkVerBinPath}x64/${filename}" )
  #   set( varVal "$ENV{WindowsSdkVerBinPath}x64/${filename}" )
  #   set( ${varName} ${varVal} CACHE FILEPATH ${docStr} FORCE )
  #   return()
  # endif()

  message( FATAL_ERROR
           "Could not find ${filename} at path: " ${varVal} ", "
           "please check ${varName} configuration, or try running CMake on the "
           "Developer Command Prompt for VS and we will try to guess the windows sdk bin for you "
           "or download https://github.com/microsoft/DirectXShaderCompiler/ to "
           "C:/Program Files (x86)/DXC/bin/" )

endfunction()

_define_dx_path( TAC_DXIL_DLL_PATH       "dxil.dll"       )
_define_dx_path( TAC_DXCOMPILER_DLL_PATH "dxcompiler.dll" )

target_compile_definitions(TacDX PUBLIC TAC_DXIL_DLL_PATH="${TAC_DXIL_DLL_PATH}")
target_compile_definitions(TacDX PUBLIC TAC_DXCOMPILER_DLL_PATH="${TAC_DXCOMPILER_DLL_PATH}")

# Bad things (shader corruption) may happen if one dll is copied but not the other
add_custom_command( TARGET TacDX
                    PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${TAC_DXIL_DLL_PATH}
                            ${TAC_DXCOMPILER_DLL_PATH}
                            $<TARGET_FILE_DIR:TacDX> )



