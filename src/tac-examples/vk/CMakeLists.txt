
if( TARGET TacVkRenderer )
else()
  return()
endif()



function( tac_target )
  if( WIN32 )
    add_executable( ${PROJECT_NAME} WIN32)
  else()
    add_executable( ${PROJECT_NAME} )
  endif()

  target_link_libraries( ${PROJECT_NAME}
    TacEngineCore
    TacECS
    TacDesktopApp
    TacVkRenderer 
  )

  target_compile_definitions( ${PROJECT_NAME} PRIVATE TAC_USE_RENDERER_VK )

  target_sources( ${PROJECT_NAME} PRIVATE
    "tac_vk_standalone_1_tri.cpp"
  )

  #target_precompile_headers( ${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

endfunction()

if( WIN32 )
  project( "standalone_win_vk_1_tri" CXX)
  tac_target()

  target_link_libraries( ${PROJECT_NAME}
    TacWinVk 
    TacWin32
    TacWin32Main
  )

  # target_compile_definitions( ${PROJECT_NAME} PRIVATE TAC_USE_RENDERER_DX11 )

endif() # if( WIN32 )


find_package( SDL2 QUIET )
if( SDL2_FOUND )
  project( "standalone_sdl_vk_1_tri" CXX)
  tac_target()

  target_sources( ${PROJECT_NAME} PRIVATE
    ${TAC_SDL_MAIN_FILES}
    #"${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.cpp"
    #"${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.h"
  )

  target_link_libraries( ${PROJECT_NAME}
    ${SDL2_LIBRARY}
    TacSDLVk
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


