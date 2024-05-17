include( CMakePrintHelpers )

set( TAC_DEBUGGING FALSE )

add_library( TacExamples )

target_sources( TacExamples PRIVATE
  "tac_examples.cpp"
  "tac_examples.h"
  "tac_examples_registry.cpp"
  "tac_examples_registry.h"
  "tac_examples_state_machine.cpp"
  "tac_examples_state_machine.h"
)

target_link_libraries( TacExamples PUBLIC TacECS )

add_subdirectory( dx12 )
add_subdirectory( render_tutorial )
add_subdirectory( fluid )
add_subdirectory( meta )
add_subdirectory( phy_sim )
add_subdirectory( vk )
add_subdirectory( text )
add_subdirectory( imgui )

target_include_directories( TacExamples PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )


set_target_properties( TacExamples PROPERTIES FOLDER "Examples" )

function( tac_target )
  if( WIN32 )
    add_executable( ${PROJECT_NAME} WIN32 )
  else()
    add_executable( ${PROJECT_NAME} )
  endif()

  set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER "Examples" )

  target_link_libraries( ${PROJECT_NAME}
    PUBLIC
    TacEngineCore
    TacDesktopApp
    TacExamples
    TacECS
  )
  # append the vulkan library if it was built...
  if(TARGET TacVkRenderer)
    
    target_link_libraries( ${PROJECT_NAME} PUBLIC TacVkRenderer )
  else()
    message("not including vulkan renderer in game-examples")
  endif()


endfunction()


if( WIN32 )
  project( "TacExamplesWin32" CXX)
  tac_target()



  #target_precompile_headers( ${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

  target_link_libraries( ${PROJECT_NAME} PUBLIC
    TacWin32
    TacWin32Main
  )
  
  if(TARGET TacVkRenderer)
    target_link_libraries( ${PROJECT_NAME} PUBLIC TacWinVk )
    #target_sources( ${PROJECT_NAME} PRIVATE
    #    "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.cpp"
    #    "${CMAKE_SOURCE_DIR}/src/shell/tac_win_vk.h"
    #    )
  endif()

endif()


find_package( SDL2 QUIET )
if( SDL2_FOUND )
  project( "TacExamplesSDL" CXX)
  tac_target()

  target_sources( ${PROJECT_NAME} PRIVATE ${TAC_SDL_MAIN_FILES} )

  
  if(TARGET TacVkRenderer)
    target_link_libraries( ${PROJECT_NAME} PUBLIC TacSDLVk )
    #target_sources( ${PROJECT_NAME} PRIVATE
    #     "${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.cpp"
    #     "${CMAKE_SOURCE_DIR}/src/shell/tac_sdl_vk.h"
    #     )
  endif()

  #target_precompile_headers( ${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

  target_include_directories( ${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIR} )

  # this was missing before, right?
  target_link_libraries( ${PROJECT_NAME} PUBLIC ${SDL2_LIBRARY} TacSDL  )

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
      target_link_libraries( ${PROJECT_NAME} PUBLIC ${DL_LIBS} )
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
    message( "sdl2 first library: ${SDL_FIRST_LIBRARY}"  )
    message( "sdl2 dll folder:    ${SDL_DLL_FOLDER}"     )
    message( "sdl2 dll full name: ${SDL2_DLL_FULL_NAME}" )
  endif()

  if( WIN32 )
    # CMAKE_COMMAND is the full path to the cmake executable
    # -E invokes command line tool mode
    # https://cmake.org/cmake/help/latest/manual/cmake.1.html
    #
    # In visual studio, this takes the form as a Post-Build Event
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_DLL_FULL_NAME}
        $<TARGET_FILE_DIR:${PROJECT_NAME}> # directory of target named ${PROJECT_NAME}
    )
  endif()
endif()

