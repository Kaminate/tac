find_package( SDL2 )
if( NOT SDL2_FOUND )
  return()
endif()


project( TacSDL CXX )
set( TAC_LIBRARIES )

add_library( TacSDL "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.cpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.cpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.h" )

target_include_directories( TacSDL PUBLIC ${SDL2_INCLUDE_DIR} )
# target_link_libraries( TacSDL ${SDL2_LIBRARY} )

set_target_properties( TacSDL PROPERTIES LINKER_LANGUAGE CXX )
if( WIN32 )
  add_definitions( /ZI ) # produce separate pdb file ( implies /debug )
endif()
target_link_libraries( TacSDL TacCommon )

