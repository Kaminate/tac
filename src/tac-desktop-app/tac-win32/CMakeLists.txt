if( NOT WIN32 )
  return()
endif()

add_library( TacWin32 )

add_subdirectory( desktopwindow )
add_subdirectory( input )
add_subdirectory( net )
add_subdirectory( tac-dx )
add_subdirectory( event )
add_subdirectory( filedialog )
add_subdirectory( os )

target_sources( TacWin32 PRIVATE
  "tac_win32.cpp"
  "tac_win32.h"
  "tac_win32_com_ptr.h"
)

# add main subdirectory after, which depends on TacWin32 target
add_subdirectory( main )

# target_precompile_headers( TacWin32 PRIVATE 
#   "${CMAKE_SOURCE_DIR}/tac-std-lib/tac_precompiled_header.h"
# )

target_link_libraries( TacWin32 TacEngineCore )

target_link_libraries( TacDX TacWin32 )
target_link_libraries( TacDX TacStdLib )
target_link_libraries( TacWin32Main TacDX )

# pix nuget test
if( TRUE )

  include( ${CMAKE_SOURCE_DIR}/cmake/pix.cmake )
  add_pix_target()
  target_link_libraries( TacWin32 PIX )

  add_custom_command( TARGET TacWin32
                      PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${PIX_DLL_PATH}
                              $<TARGET_FILE_DIR:TacWin32> )

  target_compile_definitions( TacWin32 PUBLIC TAC_PIX_NUGET )


endif()


# this is used in tac_register_renderers for __has_include( "tac_renderer_directx11.h" )
target_include_directories( TacWin32 PUBLIC renderer )

target_include_directories( TacWin32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

# Executables include the main files directory into their source
#add_library( TacWin32Main OBJECT
  #"tac_win32_main.cpp"
  #"tac_win32_main.h"
#)

# target_include_directories( TacWin32Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
#target_link_libraries( TacWin32Main TacEngineCore )

