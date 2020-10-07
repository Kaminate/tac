

set( TAC_DEBUGGING FALSE )
set( TAC_LIBRARIES 
CommonLibrary  
  Ghost
)
set( TAC_INCLUDE_DIRECTORIES "" )
set( TAC_SOURCES "" )
set( TAC_EXE_NAME "Gravestory" )
set( TAC_SOURCES 
  ${TAC_SOURCES}
  "${CMAKE_SOURCE_DIR}/src/game/tacGame.cpp"
  "${CMAKE_SOURCE_DIR}/src/game/tacGame.h"
  "${CMAKE_SOURCE_DIR}/src/shell/tacDesktopWindowManager.cpp"
  "${CMAKE_SOURCE_DIR}/src/shell/tacDesktopWindowManager.cpp"
  "${CMAKE_SOURCE_DIR}/src/shell/tacDesktopApp.cpp"
  "${CMAKE_SOURCE_DIR}/src/shell/tacDesktopApp.h" )
if( WIN32 )
  set( TAC_SOURCES 
    ${TAC_SOURCES}
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacDXGI.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacDXGI.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacXInput.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacXInput.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacNetWinsock.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacNetWinsock.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacRendererDirectX12.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacRendererDirectX12.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacRendererDirectX11.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacRendererDirectX11.h" )
endif()

# note: variables names are case-sensitive

set( TAC_INCLUDE_DIRECTORIES "" )
set( TAC_INCLUDE_DIRECTORIES
    ${TAC_INCLUDE_DIRECTORIES}
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src" )

set( TAC_SOURCES_BASE ${TAC_SOURCES} )
set( TAC_LIBRARIES_BASE ${TAC_LIBRARIES} )
set( TAC_INCLUDE_DIRECTORIES_BASE ${TAC_INCLUDE_DIRECTORIES} )

# cached variables are bullshit ( can't get cleared in a parent scope from inside a function? )
# properties suck
# ...guess we just goota use PARENT_SCOPE
function( tac_target TAC_TARGET_SUFFIX )
  # okay so like functions open up a new scope
  # this means we can't set variables normally
  # thus, everything is global
  set( TAC_TARGET_NAME "${TAC_EXE_NAME}${TAC_TARGET_SUFFIX}" PARENT_SCOPE )
  set( TAC_INCLUDE_DIRECTORIES ${TAC_INCLUDE_DIRECTORIES_BASE} PARENT_SCOPE )
  set( TAC_SOURCES  ${TAC_SOURCES_BASE} PARENT_SCOPE )
  set( TAC_LIBRARIES ${TAC_LIBRARIES_BASE} PARENT_SCOPE )
endfunction()

if( WIN32 )
  tac_target( "Win32" )
  set( TAC_SOURCES 
    ${TAC_SOURCES}
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32MouseEdge.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32MouseEdge.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32DesktopWindowManager.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32DesktopWindowManager.h"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32Main.cpp"
    "${CMAKE_SOURCE_DIR}/src/shell/windows/tacWin32Main.h" )
  include_directories( ${TAC_INCLUDE_DIRECTORIES} )

  # the win32 here changes the
  # configuration properties --> linker --> system --> subsystem
  # from console to windows, which makes the console window go away
  add_executable( ${TAC_TARGET_NAME} WIN32 ${TAC_SOURCES} )
  target_link_libraries( ${TAC_TARGET_NAME} ${TAC_LIBRARIES} )
  # target_sources( ${TAC_TARGET_NAME} PRIVATE ${Common} )
  set_target_properties( ${TAC_TARGET_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VS_STARTUP_PROJECT ${TAC_TARGET_NAME}
    OUTPUT_NAME "${TAC_EXE_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}/${TAC_TARGET_NAME}"
    LINKER_LANGUAGE CXX )
endif()



