if( NOT WIN32 )
  return()
endif()

add_executable( TacGravestory WIN32 
  "tac_game.cpp"
  "tac_game.h"
)

target_precompile_headers( TacGravestory PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

target_link_libraries( TacGravestory
  TacCore
  TacGhost
  TacWin
  TacDesktopApp
  TacWinMain
)

if(TARGET TacVkRenderer)

  if (TARGET TacSDLVk)
    target_link_libraries( TacGravestory TacSDLVk )
  endif()

  target_link_libraries( TacGravestory TacVkRenderer  )
  target_sources( TacGravestory PRIVATE
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.cpp"
      "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.h"
    )
endif()



