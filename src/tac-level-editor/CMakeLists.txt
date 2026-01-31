set( TAC_DEBUGGING FALSE )

# only build editor for desktop ( win32, unix ) platforms, ie not ps5
if(NOT (WIN32 OR UNIX) )
  return()
endif()


function( tac_target )
  if( WIN32 )
    add_executable( ${TAC_TARGET_NAME} WIN32 )
    target_link_libraries( ${TAC_TARGET_NAME} TacWin32  ) 
  else()
    add_executable( ${TAC_TARGET_NAME})
  endif()

  set_target_properties( ${TAC_TARGET_NAME} PROPERTIES FOLDER "Programs" )

  target_include_directories( ${TAC_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

  if( TARGET TacVkRenderer )
    target_link_libraries( ${TAC_TARGET_NAME} TacVkRenderer TacWinVk )

  endif()

  target_link_libraries( ${TAC_TARGET_NAME} 
    TacEngineCore
    TacDesktopApp
    TacECS )


  target_sources( ${TAC_TARGET_NAME} PRIVATE 
    "tac_level_editor.cpp"
    "tac_level_editor.h"
    "tac_level_editor_app.cpp"
    "tac_level_editor_app.h"
  )

  add_subdirectory( gizmo )
  add_subdirectory( misc )
  add_subdirectory( picking )
  add_subdirectory( prefab )
  add_subdirectory( render )
  add_subdirectory( selection )
  add_subdirectory( tools )
  add_subdirectory( windows )

endfunction()

find_package( SDL2 QUIET )
if( SDL2_FOUND AND FALSE )
  set( TAC_TARGET_NAME "LevelEditorSDL"  CACHE STRING "" FORCE)

  tac_target()

  target_sources( ${TAC_TARGET_NAME} ${TAC_SDL_MAIN_FILES} )


  #target_precompile_headers( ${TAC_TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

  target_include_directories( ${TAC_TARGET_NAME} PUBLIC 
    ${SDL2_INCLUDE_DIR} 
    # "${CMAKE_SOURCE_DIR}/include"
    # "${CMAKE_SOURCE_DIR}/src"
  )


  # this was missing before, right?
  target_link_libraries( ${TAC_TARGET_NAME} 
    ${SDL2_LIBRARY}
    TacSDLLib 
  )

  # Add the dl library for linux (dlopen, dlclose) used by gl3w.cpp
  #
  # variable name | value
  # --------------+--------------------
  # CMAKE_DL_LIBS | dl
  # DL_LIBS       | /usr/lib64/libdl.so
  #
  if( CMAKE_DL_LIBS )
    find_library( DL_LIBS ${CMAKE_DL_LIBS} )
    if( DL_LIBS )
      target_link_libraries( ${TAC_TARGET_NAME} ${DL_LIBS} )
    endif()
  endif()


  # When we run the exe, it needs sdl2.dll to function ( not to compile )
  # As a post-build step, copy sdl2.dll to the exe output directory.
  # SDL2_LIBRARY contains comma-separated filepaths to sdl2main.lib and sdl2.lib
  # ( comma separated items denote a list in cmake )

  if( ${TAC_DEBUGGING} )
    message( "sdl2 libraries: ${SDL2_LIBRARY}")
  endif()

  list( GET SDL2_LIBRARY 0 SDL_FIRST_LIBRARY)
  get_filename_component( SDL_DLL_FOLDER ${SDL_FIRST_LIBRARY} DIRECTORY )
  set( SDL2_DLL_FULL_NAME "${SDL_DLL_FOLDER}/SDL2.dll" )

  if( ${TAC_DEBUGGING} )
    message( "sdl2 first library: ${SDL_FIRST_LIBRARY}")
    message( "sdl2 dll folder: ${SDL_DLL_FOLDER}")
    message( "sdl2 dll full name: ${SDL2_DLL_FULL_NAME}")
  endif()

  # CMAKE_COMMAND is the full path to the cmake executable
  # -E invokes command line tool mode
  # https://cmake.org/cmake/help/latest/manual/cmake.1.html
  #
  # In visual studio, this takes the form as a Post-Build Event
  #
  # This must be after the add_executable and tac_target calls
  ADD_CUSTOM_COMMAND(TARGET ${TAC_TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${SDL2_DLL_FULL_NAME}
      $<TARGET_FILE_DIR:${TAC_TARGET_NAME}>)
endif() # if( SDL2_FOUND )


if( WIN32 )
  set( TAC_TARGET_NAME "LevelEditorWin32"  CACHE STRING "" FORCE)

  tac_target()


  # broken --> target_precompile_headers(${TAC_TARGET_NAME} REUSE_FROM CommonLibrary)
  # target_precompile_headers( ${TAC_TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

  target_include_directories( ${TAC_TARGET_NAME} PUBLIC 
    ${TAC_INCLUDE_DIRECTORIES} 
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src" )
  target_link_libraries( ${TAC_TARGET_NAME} TacWin32Main )

endif()


