
find_package( Vulkan )
if( NOT Vulkan_FOUND )
  # if it fails to find vulkan, it emits the message
  # Could NOT find Vulkan (missing: Vulkan_LIBRARY Vulkan_INCLUDE_DIR) (found version "")
  return()
endif()

add_library(TacVkRenderer)

add_subdirectory(thirdparty)

project( TacVkRenderer CXX )

target_link_libraries( TacVkRenderer TacCommon )
target_link_libraries( TacVkRenderer Vulkan::Vulkan )

target_sources(TacVkRenderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.cpp"
                                     "${CMAKE_CURRENT_SOURCE_DIR}/vk_types.h"
                              PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.h" )

target_precompile_headers( TacVkRenderer PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

set_target_properties(TacVkRenderer PROPERTIES LINKER_LANGUAGE CXX)

if( WIN32 )
  #  list( APPEND TAC_SOURCES "${CMAKE_SOURCE_DIR}/src/shell/windows/tac_renderer_vulkan_win32.cpp" )
  add_definitions(/ZI) # produce separate pdb file ( implies /debug )
endif()

# [ ] Q: is this needed to go to definition on vulkan/vulkan.h?
target_include_directories( TacVkRenderer SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS} )

# this is used in tac_register_renderers for __has_include( "tac_renderer_vulkan.h" )
target_include_directories( TacVkRenderer PUBLIC . )


