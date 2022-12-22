if( NOT WIN32 )
  return()
endif()

set( TAC_TARGET_NAME "GravestoryWin32" )

add_executable( ${TAC_TARGET_NAME} WIN32 )
target_sources( ${TAC_TARGET_NAME} PRIVATE
  "tac_game.cpp"
  "tac_game.h"
  ${TAC_WIN_MAIN_FILES}
  ${TAC_DESKTOP_APP_FILES}
)

target_precompile_headers( ${TAC_TARGET_NAME} PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

target_compile_options( ${TAC_TARGET_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue
target_link_libraries( ${TAC_TARGET_NAME} TacCommon TacGhost TacWin)
set_target_properties( ${TAC_TARGET_NAME} PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  VS_STARTUP_PROJECT ${TAC_TARGET_NAME}
  RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/${TAC_TARGET_NAME}"
  LINKER_LANGUAGE CXX )


