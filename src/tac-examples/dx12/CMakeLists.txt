if( NOT WIN32 )
  return()
endif()

add_executable( dx12_tutorial_1_window WIN32 
                "tac_dx12_tutorial_1_window.cpp"
                "tac_dx12_tutorial_1_window.h" )

add_executable( dx12_tutorial_2_triangle WIN32 
                "tac_dx12_tutorial_2_triangle.cpp"
                "tac_dx12_tutorial_2_triangle.h" )

add_executable( dx12_tutorial_3_texture WIN32 
                "tac_dx12_tutorial_3_texture.cpp"
                "tac_dx12_tutorial_3_texture.h" )

add_executable( dx12_tutorial_4_bundle WIN32 
                "tac_dx12_tutorial_4_bundle.cpp"
                "tac_dx12_tutorial_4_bundle.h" )

add_executable( dx12_tutorial_5_const_buf WIN32 
                "tac_dx12_tutorial_5_const_buf.cpp"
                "tac_dx12_tutorial_5_const_buf.h" )

add_executable( dx12_tutorial_6_frame_buf WIN32 
                "tac_dx12_tutorial_6_frame_buf.cpp"
                "tac_dx12_tutorial_6_frame_buf.h" )

get_property( exe_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )

add_library( TacDX12ShaderCompile
  tac_dx12_tutorial_shader_compile.cpp
  tac_dx12_tutorial_shader_compile.h
  tac_dx12_tutorial_2_dxc.cpp
  tac_dx12_tutorial_2_dxc.h
)

target_link_libraries( TacDX12ShaderCompile TacWin32 TacRHI TacStdLib )

add_library( TacDX12ExampleHelpers
  tac_dx12_tutorial_command_queue.cpp
  tac_dx12_tutorial_command_queue.h
  tac_dx12_tutorial_gpu_upload_allocator.cpp
  tac_dx12_tutorial_gpu_upload_allocator.h
  tac_dx12_tutorial_input_layout_builder.cpp
  tac_dx12_tutorial_input_layout_builder.h
  tac_dx12_tutorial_root_sig_builder.cpp
  tac_dx12_tutorial_root_sig_builder.h
  tac_dx12_tutorial_checkerboard.cpp
  tac_dx12_tutorial_checkerboard.h
  tac_dx12_tutorial.cpp
  tac_dx12_tutorial.h
  tac_dx12_tutorial_command_allocator_pool.cpp
  tac_dx12_tutorial_command_allocator_pool.h
  tac_dx12_tutorial_context_manager.cpp
  tac_dx12_tutorial_context_manager.h
  tac_dx12_tutorial_fence.h
)

target_link_libraries( TacDX12ExampleHelpers TacWin32 TacStdLib TacDesktopApp )

foreach( targetname ${exe_targets} )
  target_link_libraries( ${targetname}
    TacStdLib
    TacEngineCore
    TacECS
    TacDesktopApp
    TacWin32
    TacWin32Main
    TacDX12ShaderCompile
    TacDX12ExampleHelpers
  )
endforeach()

get_property( all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )
foreach( targetname ${all_targets} )
  set_target_properties( ${targetname} PROPERTIES FOLDER "D3D12 Tutorial" )
endforeach()

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

# Bad things (shader corruption) may happen if one dll is copied but not the other
add_custom_command( TARGET TacDX12ShaderCompile
                    PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${TAC_DXIL_DLL_PATH}
                            ${TAC_DXCOMPILER_DLL_PATH}
                            $<TARGET_FILE_DIR:TacDX12ShaderCompile> )




