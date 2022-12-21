if( WIN32 )

  set( Target "TacWinLib" )

  project( ${Target} CXX )
  set( TAC_LIBRARIES TacCommon  )

  set("TAC_WIN_LIB_FILES"
  "${CMAKE_CURRENT_SOURCE_DIR}/net/tac_net_winsock.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/net/tac_net_winsock.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_dxgi.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_dxgi.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx11.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx11.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx11_shader_preprocess.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx12.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx12.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_directx_pix.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_opengl4_win32.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer/tac_renderer_vulkan_win32.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/desktopwindow/tac_win32_desktop_window_manager.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/desktopwindow/tac_win32_desktop_window_manager.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/input/tac_win32_mouse_edge.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/input/tac_win32_mouse_edge.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/input/tac_xinput.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/input/tac_xinput.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_win32.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_win32.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/tac_win32_os.cpp"
# CACHE INTERNAL ""
  )
  add_library( ${Target} ${TAC_WIN_LIB_FILES} )

  target_precompile_headers( ${Target} PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
  )

  set_target_properties( ${Target} PROPERTIES LINKER_LANGUAGE CXX )
  add_definitions( /ZI ) # produce separate pdb file ( implies /debug )
  target_link_libraries( ${Target} ${TAC_LIBRARIES} )


# this is used in tac_register_renderers for __has_include( "tac_renderer_directx11.h" )
  target_include_directories( ${Target} PUBLIC renderer )

endif() # if WIN32
