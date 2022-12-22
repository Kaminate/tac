if( NOT WIN32 )
  return()
endif()

set( Target "TacWinLib" )
project( TacWinLib CXX )

add_library( TacWinLib )

add_subdirectory(desktopwindow)
add_subdirectory(input)
add_subdirectory(net)
add_subdirectory(renderer)

target_sources(TacWinLib PRIVATE
  "tac_win32.cpp"
  "tac_win32.h"
  "tac_win32_os.cpp"
)

target_precompile_headers( TacWinLib PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

set_target_properties( TacWinLib PROPERTIES LINKER_LANGUAGE CXX )
add_definitions( /ZI ) # produce separate pdb file ( implies /debug )
target_link_libraries( TacWinLib TacCommon )

# this is used in tac_register_renderers for __has_include( "tac_renderer_directx11.h" )
target_include_directories( TacWinLib PUBLIC renderer )

