
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

  find_package( SDL2 QUIET )
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


add_subdirectory( tac-win32 )
add_subdirectory( opengl )
add_subdirectory( sdl )
add_subdirectory( vulkan )


# add_library( TacDesktopApp OBJECT )
add_library( TacDesktopApp )
target_sources( TacDesktopApp
  PRIVATE
  "tac_render_state.cpp"
  "tac_logic_thread.cpp"
  "tac_logic_thread.h"
  "tac_platform_thread.cpp"
  "tac_platform_thread.h"
  PUBLIC
  "tac_iapp.h"
  "tac_render_state.h"
)

add_subdirectory( desktop_app )
add_subdirectory( desktop_window )
add_subdirectory( desktop_event )

target_include_directories( TacDesktopApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
target_link_libraries( TacDesktopApp TacECS )

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
