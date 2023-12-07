if( NOT WIN32 )
  return()
endif()

add_executable( standalone_win_dx12_1_window WIN32 
  "tac_example_dx12_1_window.cpp"
  "tac_example_dx12_1_window.h"
)

add_executable( standalone_win_dx12_2_triangle WIN32 
  "tac_example_dx12_2_triangle.cpp"
  "tac_example_dx12_2_triangle.h"
)

# Get a semicolon-separated list of executable target names
get_property(targetnames DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)

foreach(targetname ${targetnames})
  target_sources(${targetname} PRIVATE ${TAC_WIN_MAIN_FILES})
  target_link_libraries( ${targetname}
    TacCommon
    TacGhost
    TacWin
    $<TARGET_OBJECTS:TacDesktopApp>
  )

  if( FALSE )
    ADD_CUSTOM_COMMAND(
      TARGET ${targetname}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "dxcompiler.dll"
        $<TARGET_FILE_DIR:${targetname}>
    )
  endif()

  if( FALSE )
    ADD_CUSTOM_COMMAND(
      TARGET ${targetname}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "dxil.dll"
        $<TARGET_FILE_DIR:${targetname}>
    )
  endif()
endforeach()

if( false )
  add_custom_command(
    output "${output}"
    depends "${input}"
    command ${CMAKE_COMMAND} -E copy_if_different
    "${input}" "${output}"
  )
endif()

if( TRUE )
  # The WindowsSdkVerBinPath environment variable is set by the call to vcvars64.bat in
  # tac/run/run_vs2022.bat
  set(DXIL_PATH $ENV{WindowsSdkVerBinPath}x64/dxil.dll)

  if(NOT EXISTS ${DXIL_PATH} )
    message( FATAL_ERROR "cannot find std modules .props")
  endif()
  message( VERBOSE "found dxil at ${DXIL_PATH}")

  # In a program that is using dxc to compile shaders via `#include <dxcapi.h>` and
  # `#pragma comment (lib, "dxcompiler.lib" )`, the call to `IDxcCompiler::Compile` will emit the
  # following warning if dxil.dll is not in the same folder as the target executable:
  #
  #   DXIL.dll not found. Resulting DXIL will not be signed for use in release environments

  # Copy dxil.dll to the same folder as the target executable
  add_custom_command(
    TARGET standalone_win_dx12_2_triangle
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${DXIL_PATH}
      $<TARGET_FILE_DIR:standalone_win_dx12_2_triangle>
  )
endif()

