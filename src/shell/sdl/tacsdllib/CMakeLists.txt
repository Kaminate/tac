find_package( SDL2 )
if( SDL2_FOUND )


  set(Target "TacSDLLib")

  project( ${Target} CXX )
  set( TAC_LIBRARIES CommonLibrary  )

  set("TAC_SDL_LIB_FILES"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_app.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_sdl_desktop_window_manager.h"
    )

  add_library( ${Target} ${TAC_SDL_LIB_FILES} )
  # include_directories( . )
  target_include_directories( ${Target} PUBLIC ${SDL2_INCLUDE_DIR} )
  # target_link_libraries( ${Target} ${SDL2_LIBRARY} )

  set_target_properties( ${Target} PROPERTIES LINKER_LANGUAGE CXX )
  if( WIN32 )
    add_definitions( /ZI ) # produce separate pdb file ( implies /debug )
  endif()
  target_link_libraries( ${Target} ${TAC_LIBRARIES} )

endif()
