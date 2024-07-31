if( NOT WIN32 )
  return()
endif()

add_executable( TacJPPT WIN32 
  "tac_jppt.cpp"
  "tac_jppt.h"
)

target_link_libraries( TacJPPT
  TacEngineCore
  TacECS
  TacWin32
  TacWin32Main
  TacDesktopApp
)


