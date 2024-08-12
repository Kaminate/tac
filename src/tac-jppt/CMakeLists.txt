if( NOT WIN32 )
  return()
endif()

add_executable( TacJPPT WIN32 
  "tac_jppt_BVH.cpp"
  "tac_jppt_BVH.h"
  "tac_jppt_Scene.cpp"
  "tac_jppt_Scene.h"
  "tac_jppt.cpp"
  "tac_jppt.h"
  "tac_jppt_cornell_box.cpp"
  "tac_jppt_cornell_box.h"
)

target_link_libraries( TacJPPT
  TacEngineCore
  TacECS
  TacWin32
  TacWin32Main
  TacDesktopApp
)


