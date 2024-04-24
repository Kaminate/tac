add_executable( render_tutorial_1_window WIN32 "tac_render_tutorial_1_window.cpp")
add_executable( render_tutorial_2_triangle WIN32 "tac_render_tutorial_2_triangle.cpp")

# temp
# target_link_libraries( standalone_win_dx12_6_frame_buf TacStdLib )

# Get a semicolon-separated list of executable target names
get_property( targetnames DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )

foreach( targetname ${targetnames} )
  message( "targetname: ${targetname}" )
  target_link_libraries( ${targetname}
    TacStdLib
    TacEngineCore
    TacECS
    TacDesktopApp
    TacWin32
    TacWin32Main
    TacDX12ShaderCompile
    TacDX12ExampleHelpers
  )
endforeach()


