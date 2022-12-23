


list( APPEND TAC_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_vk_1_tri.h"
)

set( TAC_SOURCES ${TAC_SOURCES} CACHE INTERNAL "" FORCE )

if( WIN32 )
  if( TARGET TacVkRenderer )
    project( "standalone_win_vk_1_tri" )
    add_executable( ${PROJECT_NAME} WIN32)

    target_link_libraries( ${PROJECT_NAME}
      TacCommon
      TacGhost
      $<TARGET_OBJECTS:TacDesktopApp>
      TacVkRenderer 
      TacWinVk 
      TacWin
    )

    target_compile_options( ${PROJECT_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue

    target_sources( ${PROJECT_NAME} PRIVATE
      ${TAC_WIN_MAIN_FILES}
      "vk_standalone_1_tri.cpp"
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.cpp"
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.h"
    )

    target_precompile_headers( ${PROJECT_NAME} PRIVATE 
      "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
    )

    set_target_properties( ${PROJECT_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      VS_STARTUP_PROJECT ${PROJECT_NAME}
      RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}"
      LINKER_LANGUAGE CXX )


  endif() # if( TARGET TacVkRenderer )
endif() # if( WIN32 )


