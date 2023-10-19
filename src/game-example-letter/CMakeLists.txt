include( CMakePrintHelpers )

set( TAC_DEBUGGING FALSE )

set( TAC_SOURCES 
  "tac_example_letter.cpp"
  "tac_example_letter.h"
  CACHE INTERNAL "" FORCE )

# These append to TAC_SOURCES

add_library( TacExampleLetter )
target_sources( TacExampleLetter PRIVATE ${TAC_SOURCES} )


function( tac_target )


endfunction()


if( WIN32 )
  project( "TacExampleLetterWin32" )
  add_executable( ${PROJECT_NAME} WIN32 )
  target_link_libraries( ${PROJECT_NAME}
    TacCommon
    TacGhost
    $<TARGET_OBJECTS:TacDesktopApp>
    TacExampleLetter
  )

  set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_STARTUP_PROJECT ${PROJECT_NAME}
    LINKER_LANGUAGE CXX )

  target_sources( ${PROJECT_NAME} PRIVATE ${TAC_WIN_MAIN_FILES})

  target_precompile_headers( ${PROJECT_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

  target_compile_options( ${PROJECT_NAME} PUBLIC /ZI ) # Program Database for Edit & Continue
  target_link_libraries( ${PROJECT_NAME} TacWin  )

  if(TARGET TacVkRenderer)
    target_link_libraries( ${PROJECT_NAME} TacWinVk )
    target_link_libraries( ${PROJECT_NAME} TacVkRenderer )
  endif()
  

endif()


