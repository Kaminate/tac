
if( TARGET TacVkRenderer )
else()
  return()
endif()


list( APPEND TAC_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_vk_1_tri.h"
)

set( TAC_SOURCES ${TAC_SOURCES} CACHE INTERNAL "" FORCE )

function( tac_target )
  if( WIN32 )
    add_executable( ${PROJECT_NAME} WIN32)
  else()
    add_executable( ${PROJECT_NAME} )
  endif()

  target_link_libraries( ${PROJECT_NAME}
    TacCommon
    TacGhost
    $<TARGET_OBJECTS:TacDesktopApp>
    TacVkRenderer 
  )

  target_sources( ${PROJECT_NAME} PRIVATE
    "vk_standalone_1_tri.cpp"
  )

  target_precompile_headers( ${PROJECT_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

endfunction()

if( WIN32 )
  project( "standalone_win_vk_1_tri" )
  tac_target()

  target_link_libraries( ${PROJECT_NAME}
    TacWinVk 
    TacWin
  )

  target_sources( ${PROJECT_NAME} PRIVATE
    ${TAC_WIN_MAIN_FILES}
    "vk_standalone_1_tri.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.h"
  )

  target_compile_options( ${PROJECT_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue

  set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_STARTUP_PROJECT ${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}"
    LINKER_LANGUAGE CXX )

endif() # if( WIN32 )


find_package( SDL2 )
if( SDL2_FOUND )
  project( "standalone_sdl_vk_1_tri" )
  tac_target()

  target_sources( ${PROJECT_NAME} PRIVATE
    ${TAC_SDL_MAIN_FILES}
    "${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.h"
  )

  target_link_libraries( ${PROJECT_NAME}
    TacSDLVk
    ${SDL2_LIBRARY}
    TacSDL 
  )


  target_include_directories( ${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIR} )

  # Add the dl library for linux (dlopen, dlclose) used by gl3w.cpp
  # ${CMAKE_DL_LIBS} == dl
  # ${DL_LIBS} == /usr/lib64/libdl.so
  if( CMAKE_DL_LIBS )
    find_library( DL_LIBS ${CMAKE_DL_LIBS} )
    if( DL_LIBS )
      target_link_libraries( ${PROJECT_NAME} ${DL_LIBS} )
    endif()
  endif()



endif()


