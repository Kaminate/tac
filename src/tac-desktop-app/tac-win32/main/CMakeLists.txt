# Executables include the main files directory into their source
add_library( TacWin32Main OBJECT
  "tac_win32_main.cpp"
  "tac_win32_main.h"
)

target_link_libraries( TacWin32Main TacEngineCore )
target_link_libraries( TacWin32Main TacWin32 )
