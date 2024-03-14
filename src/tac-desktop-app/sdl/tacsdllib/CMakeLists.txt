find_package( SDL2 QUIET )
if( NOT SDL2_FOUND )
  return()
endif()


add_library( TacSDL "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.cpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.cpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.h" )

target_include_directories( TacSDL PUBLIC ${SDL2_INCLUDE_DIR} )
# target_link_libraries( TacSDL ${SDL2_LIBRARY} )

target_link_libraries( TacSDL TacCommon )

