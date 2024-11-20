add_executable( render_tutorial_1_window   WIN32
  "tac_render_tutorial_1_window.cpp"
  "tac_render_tutorial_1_window.h")
add_executable( render_tutorial_2_triangle WIN32
  "tac_render_tutorial_2_triangle.cpp"
  "tac_render_tutorial_2_triangle.h")
add_executable( render_tutorial_3_texture  WIN32
  "tac_render_tutorial_3_texture.cpp"
  "tac_render_tutorial_3_texture.h")
add_executable( render_tutorial_4_const_buf  WIN32
  "tac_render_tutorial_4_const_buf.cpp"
  "tac_render_tutorial_4_const_buf.h" )

get_property( targetnames DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )
foreach( targetname ${targetnames} )
  target_sources ( ${targetname} PRIVATE
    tac_render_tutorial.cpp 
    tac_render_tutorial.h
  )

  set_target_properties( ${targetname} PROPERTIES FOLDER "Programs/Render Tutorial" )

  target_link_libraries( ${targetname}
    TacStdLib
    TacEngineCore
    TacDesktopApp
    TacWin32
    TacWin32Main
    TacDX12ShaderCompile
    TacDX12ExampleHelpers
  )
endforeach()


