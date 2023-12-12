if( NOT WIN32 )
  return()
endif()

project("GravestoryWin32" CXX)

add_executable( ${PROJECT_NAME} WIN32 )
target_sources( ${PROJECT_NAME} PRIVATE
  "tac_game.cpp"
  "tac_game.h"
  ${TAC_WIN_MAIN_FILES}
)

target_precompile_headers( ${PROJECT_NAME} PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

target_compile_options( ${PROJECT_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue
target_link_libraries( ${PROJECT_NAME}
  TacCommon
  TacGhost
  TacWin
  $<TARGET_OBJECTS:TacDesktopApp>
)

if(TARGET TacVkRenderer)

  if (TARGET TacSDLVk)
    target_link_libraries( ${PROJECT_NAME} TacSDLVk )
  endif()

  target_link_libraries( ${PROJECT_NAME} TacVkRenderer  )
  target_sources( ${PROJECT_NAME} PRIVATE
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.cpp"
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.h"
      )
endif()

set_target_properties( ${PROJECT_NAME} PROPERTIES
  #VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  VS_STARTUP_PROJECT ${PROJECT_NAME}
  RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}"
  LINKER_LANGUAGE CXX )


