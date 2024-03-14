if( NOT WIN32 )
  return()
endif()

add_library( TacWin32 )

add_subdirectory( desktopwindow )
add_subdirectory( input )
add_subdirectory( net )
add_subdirectory( renderer )

target_sources( TacWin32 PRIVATE
  "tac_win32.cpp"
  "tac_win32.h"
  "tac_win32_event.cpp"
  "tac_win32_event.h"
  "tac_win32_os.cpp"
  "tac_win32_os.h"
  "tac_win32_com_ptr.h"
  "tac_win32_file_dialog.h"
  "tac_win32_file_dialog.cpp"
)

# target_precompile_headers( TacWin32 PRIVATE 
#   "${CMAKE_SOURCE_DIR}/tac-std-lib/tac_precompiled_header.h"
# )


target_link_libraries( TacWin32 TacEngineCore )

# this is used in tac_register_renderers for __has_include( "tac_renderer_directx11.h" )
target_include_directories( TacWin32 PUBLIC renderer )

target_include_directories( TacWin32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

# Executables include the main files directory into their source
add_library( TacWin32Main OBJECT
  "tac_win32_main.cpp"
  "tac_win32_main.h"
)

target_include_directories( TacWin32Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
target_link_libraries( TacWin32Main TacEngineCore )

