include( CMakePrintHelpers )

set( TAC_DEBUGGING FALSE )

set( TAC_EXAMPLE_FILES 
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_meta.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_meta.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_fluid.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_fluid.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_1_force.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_1_force.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_2_integration.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_2_integration.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_3_torque.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_3_torque.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_4_tank.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_4_tank.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_5_lincollision.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_5_lincollision.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_6_rotcollision.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_6_rotcollision.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_7_friction.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_7_friction.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_registry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_registry.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_presentation.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_presentation.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_state_machine.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples_state_machine.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_examples.h"
)

function( tac_target TAC_TARGET_SUFFIX )
  # okay so like functions open up a new scope
  # this means we can't set variables normally
  # thus, everything is global
  set( TAC_EXE_NAME            "TacExamples${TAC_TARGET_SUFFIX}" CACHE STRING "" FORCE)
  set( TAC_TARGET_NAME         "${TAC_EXE_NAME}"                 CACHE STRING "" FORCE)

  if( TAC_DEBUGGING )
    message( "tac exe name: ${TAC_EXE_NAME}" )
    message( "tac tgt name: ${TAC_TARGET_NAME}" )
  endif()

  # the win32 here changes the
  # configuration properties --> linker --> system --> subsystem
  # from console to windows, which makes the console window go away
  #
  # If this is a unix build, will the WIN32 here fuck everything up?
  if( WIN32 )
    add_executable( ${TAC_TARGET_NAME} WIN32 ${TAC_SOURCES} )
  else()
    add_executable( ${TAC_TARGET_NAME} ${TAC_SOURCES} )
  endif()

  target_sources(${TAC_TARGET_NAME} PRIVATE ${TAC_EXAMPLE_FILES}
                                            ${TAC_DESKTOP_APP_FILES} )

  target_include_directories( ${TAC_TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include"
                                                         "${CMAKE_SOURCE_DIR}/src" )

  target_link_libraries( ${TAC_TARGET_NAME} TacCommon TacGhost )
  # append the vulkan library if it was built...
  if(TARGET TacVkRenderer)
    target_link_libraries( ${TAC_TARGET_NAME} TacVkRenderer  )
  else()
    message("not including vulkan renderer in game-examples")
  endif()


  set_target_properties( ${TAC_TARGET_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_STARTUP_PROJECT ${TAC_TARGET_NAME}
    LINKER_LANGUAGE CXX )


endfunction()


if( WIN32 )
  tac_target( "Win32" )


  target_sources(${TAC_TARGET_NAME} PRIVATE ${TAC_WIN_MAIN_FILES})

  target_precompile_headers( ${TAC_TARGET_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

  target_compile_options( ${TAC_TARGET_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue

  target_link_libraries( ${TAC_TARGET_NAME} TacWinLib  )
  if(TARGET TacVkRenderer)
    target_link_libraries( ${TAC_TARGET_NAME} TacVkRenderer  )
  endif()

endif()


find_package( SDL2 )
if( SDL2_FOUND )
  tac_target( "SDL" )
  find_package( SDL2 REQUIRED )

  list( APPEND TAC_SOURCES ${TAC_SDL_MAIN_FILES} )


  

  target_precompile_headers( ${TAC_TARGET_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

  target_include_directories( ${TAC_TARGET_NAME} PUBLIC ${SDL2_INCLUDE_DIR} )

  # this was missing before, right?
  target_link_libraries( ${TAC_TARGET_NAME} ${SDL2_LIBRARY}
                                            TacSDL  )

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
    ADD_CUSTOM_COMMAND(TARGET ${TAC_TARGET_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_DLL_FULL_NAME}
        $<TARGET_FILE_DIR:${TAC_TARGET_NAME}>)
  endif()
endif()

