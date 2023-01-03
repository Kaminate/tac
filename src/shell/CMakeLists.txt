
find_package( Vulkan )
if( Vulkan_FOUND )
  if( WIN32 )
    add_library( TacWinVk )
    target_sources( TacWinVk PRIVATE
      "tac_win_vk.cpp"
      "tac_win_vk.h"
      "tac_desktop_vk.h"
      "tac_desktop_vk.cpp"
    )
    # target_link_libraries( TacWinVk Vulkan::Vulkan )
    target_include_directories( TacWinVk SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )
  endif() # if( WIN32 )

  find_package( SDL2 )
  if( SDL2_FOUND )
    add_library( TacSDLVk )
    target_sources( TacSDLVk PRIVATE
      "tac_sdl_vk.cpp"
      "tac_sdl_vk.h"
      "tac_desktop_vk.h"
      "tac_desktop_vk.cpp"
    )
    # target_link_libraries( TacSDLVk Vulkan::Vulkan )
    target_link_libraries( TacSDLVk TacSDL )
    target_include_directories( TacSDLVk SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )
  endif() # if( SDL2_FOUND )
endif() # if( Vulkan_FOUND )


add_subdirectory( windows )
add_subdirectory( opengl )
add_subdirectory( sdl )
add_subdirectory( vulkan )

add_library( TacDesktopApp OBJECT )
target_sources( TacDesktopApp
  PRIVATE
  "tac_desktop_app.cpp"
  "tac_desktop_window_graphics.cpp"
  "tac_desktop_window_settings_tracker.cpp"
  "tac_register_renderers.cpp"
  PUBLIC
  "tac_desktop_window_graphics.h"
  "tac_desktop_window_settings_tracker.h"
  "tac_desktop_app.h"
  "tac_register_renderers.h"
)


# test begin ???
if( TARGET TacVkRenderer )
  target_compile_definitions( TacDesktopApp PRIVATE TAC_USE_RENDERER_VK )
endif()
if( WIN32 )
  target_include_directories( TacDesktopApp PRIVATE TAC_USE_RENDERER_DX11 )
endif()
# test end ???

# for tac_register_renderers
if( WIN32 )
  target_include_directories( TacDesktopApp PRIVATE "windows/renderer" )
endif()

if( Vulkan_FOUND )
  target_include_directories( TacDesktopApp PRIVATE "vulkan" )
endif()
