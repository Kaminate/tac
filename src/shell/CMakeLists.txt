
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
    target_compile_definitions( TacWinVk PRIVATE TAC_USE_DESKTOP_WIN32 )

    # target_link_libraries( TacWinVk Vulkan::Vulkan )
    target_include_directories( TacWinVk SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )
  endif() # if( WIN32 )

  find_package( SDL2 )
  if( SDL2_FOUND )

  #
  #  add_library( TacDesktopVk OBJECT )
  #  set_target_properties(TacDesktopVk PROPERTIES LINKER_LANGUAGE CXX)
  #  target_sources( TacDesktopVk
  #    # PRIVATE "tac_desktop_vk.h"
  #    PUBLIC "tac_desktop_vk.h" )
  #

    # purpose of this library, uhh:
    add_library( TacSDLVk )
    target_sources( TacSDLVk PRIVATE
      "tac_sdl_vk.cpp"
      "tac_sdl_vk.h"
      "tac_desktop_vk.h"
      "tac_desktop_vk.h" )

    # target_link_libraries( TacSDLVk Vulkan::Vulkan )
    target_link_libraries( TacSDLVk TacSDL )
    target_compile_definitions( TacSDLVk PRIVATE TAC_USE_DESKTOP_SDL )
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
  "tac_desktop_app_renderers.cpp"
  "tac_desktop_event.cpp"
  "tac_desktop_window_graphics.cpp"
  "tac_desktop_window_settings_tracker.cpp"
  PUBLIC
  "tac_desktop_app.h"
  "tac_desktop_app_renderers.h"
  "tac_desktop_event.h"
  "tac_desktop_window_graphics.h"
  "tac_desktop_window_settings_tracker.h"
)


# if( False ) here because it should not be the responsibilty of the shell to determine which
# renderers to use
if( False )

  # test begin ???
  if( TARGET TacVkRenderer )
    target_compile_definitions( TacDesktopApp PUBLIC TAC_USE_RENDERER_VK )
  endif()

  if( WIN32 )
    target_compile_definitions( TacDesktopApp PUBLIC TAC_USE_RENDERER_DX11 )
  endif()
  # test end ???
endif()

# for tac_register_renderers
if( WIN32 )
  target_include_directories( TacDesktopApp PUBLIC "windows/renderer" )
endif()

if( Vulkan_FOUND )
  target_include_directories( TacDesktopApp PUBLIC "vulkan" )
endif()
