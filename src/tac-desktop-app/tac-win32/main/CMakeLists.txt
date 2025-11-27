# Executables include the main files directory into their source
add_library( TacWin32Main OBJECT
  "tac_win32_main.cpp"
  "tac_win32_main.h"
  "tac_win32_platform.cpp"
  "tac_win32_platform.h"
  "tac_win32_redirect_stream_buf.cpp"
  "tac_win32_enable_dpi_awareness.cpp"
)

set_target_properties( TacWin32Main PROPERTIES FOLDER "Engine/Desktop App" )

target_include_directories( TacWin32Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

target_link_libraries( TacWin32Main TacEngineCore )
target_link_libraries( TacWin32Main TacWin32 )
