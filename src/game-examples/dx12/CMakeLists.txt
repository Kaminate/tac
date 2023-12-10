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

endforeach()

# see more comments at where DXIL_PATH was cached
if( NOT EXISTS ${DXIL_PATH} )
  message( FATAL_ERROR "DXIL_PATH cache var doesnt exist, did you run run_vs2022.bat?")
else()
  add_custom_command( TARGET standalone_win_dx12_2_triangle
                      PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${DXIL_PATH}
                              $<TARGET_FILE_DIR:standalone_win_dx12_2_triangle> )
endif()
