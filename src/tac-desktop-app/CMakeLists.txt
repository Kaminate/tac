


add_library( TacDesktopApp )
set_target_properties( TacDesktopApp PROPERTIES FOLDER "Engine/Desktop App" )

add_subdirectory( desktop_app )
add_subdirectory( desktop_window )
add_subdirectory( desktop_event )
add_subdirectory( desktop_thread )
add_subdirectory( tac-win32 )
add_subdirectory( sdl )

target_include_directories( TacDesktopApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
target_link_libraries( TacDesktopApp TacStdLib )

# for tac_register_renderers
if( WIN32 )
  target_include_directories( TacDesktopApp PUBLIC "windows/renderer" )
endif()

find_package( Vulkan )
if( FALSE AND Vulkan_FOUND )
  target_include_directories( TacDesktopApp PUBLIC "vulkan" )
  if( WIN32 )

    add_library( TacWinVk )
    target_sources( TacWinVk PRIVATE
      "desktop_glue/tac_win_vk.cpp"
      "desktop_glue/tac_win_vk.h"
      "desktop_glue/tac_desktop_vk.h"
      "desktop_glue/tac_desktop_vk.cpp"
    )
    target_compile_definitions( TacWinVk PRIVATE TAC_USE_DESKTOP_WIN32 )

    # target_link_libraries( TacWinVk Vulkan::Vulkan )
    target_include_directories( TacWinVk SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )
  endif() # if( WIN32 )

  find_package( SDL2 QUIET )
  if( SDL2_FOUND )

    # purpose of this library, uhh:
    add_library( TacSDLVk )
    target_sources( TacSDLVk PRIVATE
      "desktop_glue/tac_sdl_vk.cpp"
      "desktop_glue/tac_sdl_vk.h"
      "desktop_glue/tac_desktop_vk.h"
      "desktop_glue/tac_desktop_vk.h" )

    target_link_libraries( TacSDLVk TacSDL )
    target_compile_definitions( TacSDLVk PRIVATE TAC_USE_DESKTOP_SDL )
    target_include_directories( TacSDLVk SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )
  endif() # if( SDL2_FOUND )
endif() # if( Vulkan_FOUND )

