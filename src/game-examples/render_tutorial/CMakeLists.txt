add_executable( render_tutorial_1_window   WIN32 "tac_render_tutorial_1_window.cpp")
add_executable( render_tutorial_2_triangle WIN32 "tac_render_tutorial_2_triangle.cpp")
add_executable( render_tutorial_3_texture  WIN32 "tac_render_tutorial_3_texture.cpp")

get_property( targetnames DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )
foreach( targetname ${targetnames} )

  set_target_properties( ${targetname} PROPERTIES FOLDER "Render Tutorial" )

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


