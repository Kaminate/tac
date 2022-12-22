if( NOT WIN32 )
  return()
endif()

project( TacWin CXX )

add_library( TacWin )

add_subdirectory(desktopwindow)
add_subdirectory(input)
add_subdirectory(net)
add_subdirectory(renderer)

target_sources(TacWin PRIVATE
  "tac_win32.cpp"
  "tac_win32.h"
  "tac_win32_os.cpp"
)

target_precompile_headers( TacWin PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

set_target_properties( TacWin PROPERTIES LINKER_LANGUAGE CXX )
add_definitions( /ZI ) # produce separate pdb file ( implies /debug )
target_link_libraries( TacWin TacCommon )

# this is used in tac_register_renderers for __has_include( "tac_renderer_directx11.h" )
target_include_directories( TacWin PUBLIC renderer )

# Executables include the main files directory into their source
set("TAC_WIN_MAIN_FILES"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_win32_main.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_win32_main.h"
CACHE INTERNAL ""
)

