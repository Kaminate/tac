if( NOT WIN32 )
  return()
endif()

add_executable( TacGravestory WIN32 
  "tac_game.cpp"
  "tac_game.h"
)

#target_precompile_headers( TacGravestory PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h")

target_link_libraries( TacGravestory
  TacEngineCore
  TacECS
  TacWin32
  TacWin32Main
  TacDesktopApp
)

if( TARGET TacVkRenderer )

  if ( TARGET TacSDLVk )
    target_link_libraries( TacGravestory TacSDLVk )
  endif()

  target_link_libraries( TacGravestory TacVkRenderer  )
  target_link_libraries( TacGravestory TacWinVk  )
endif()



