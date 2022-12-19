#set("TAC_VULKAN_SOURCE_FILES"
#"${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.cpp"
#"${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.h"
#CACHE INTERNAL ""
#)

find_package( Vulkan )
if( Vulkan_FOUND )

  set(Target "TacVkRenderer")

  project( ${Target} CXX )
  set( TAC_LIBRARIES CommonLibrary  )

  set("TAC_VK_LIB_FILES"
  "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vkbootstrap/VkBootstrap.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vkbootstrap/VkBootstrap.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vma/vk_mem_alloc.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_renderer_vulkan.h"
  CACHE INTERNAL ""
  )
  add_library(${Target} ${TAC_VK_LIB_FILES})

  target_precompile_headers( ${Target} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

  # list( APPEND TAC_SOURCES ${Vulkan_INCLUDE_DIRS} ${TAC_VULKAN_SOURCE_FILES} )
  # if( WIN32 AND FALSE)
  #  list( APPEND TAC_SOURCES "${CMAKE_SOURCE_DIR}/src/shell/windows/tac_renderer_vulkan_win32.cpp" )
  #endif()
  list( APPEND TAC_LIBRARIES Vulkan::Vulkan )


  set_target_properties(${Target} PROPERTIES LINKER_LANGUAGE CXX)
  if( WIN32 )
    add_definitions(/ZI) # produce separate pdb file ( implies /debug )
  endif()
  target_link_libraries( ${Target} ${TAC_LIBRARIES} )


  # this is used in tac_register_renderers for __has_include( "tac_renderer_vulkan.h" )
  target_include_directories( ${Target} PUBLIC . )

# endif() # if WIN32


# Dont need a else branch, if it fails to find vulkan, it emits the message
# Could NOT find Vulkan (missing: Vulkan_LIBRARY Vulkan_INCLUDE_DIR) (found version "")
endif()

