
find_package( Vulkan )
if( NOT Vulkan_FOUND )
  # if it fails to find vulkan, it emits the message
  # Could NOT find Vulkan (missing: Vulkan_LIBRARY Vulkan_INCLUDE_DIR) (found version "")
  return()
endif()

add_library(TacVkRenderer)

add_subdirectory(thirdparty)


target_link_libraries( TacVkRenderer TacCore )
target_link_libraries( TacVkRenderer Vulkan::Vulkan )
if( TARGET TacDesktopVk)
  target_link_libraries( TacVkRenderer TacDesktopVk )
endif()

target_sources(TacVkRenderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.cpp"
                                     "${CMAKE_CURRENT_SOURCE_DIR}/tac_vk_types.h"
                              PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.h" )

target_precompile_headers( TacVkRenderer PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )


# [ ] Q: is this needed to go to definition on vulkan/vulkan.h?
target_include_directories( TacVkRenderer SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )

# this is used in tac_register_renderers for __has_include( "tac_renderer_vulkan.h" )
target_include_directories( TacVkRenderer PUBLIC . )

# ???
include_directories( . )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
