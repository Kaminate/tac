if( NOT WIN32 )
  return()
endif()

add_executable( dx12_tutorial_1_window WIN32 
                "tac_dx12_tutorial_1_window.cpp"
                "tac_dx12_tutorial_1_window.h" )

add_executable( dx12_tutorial_2_triangle WIN32 
                "tac_dx12_tutorial_2_triangle.cpp"
                "tac_dx12_tutorial_2_triangle.h" )

add_executable( dx12_tutorial_3_texture WIN32 
                "tac_dx12_tutorial_3_texture.cpp"
                "tac_dx12_tutorial_3_texture.h" )

add_executable( dx12_tutorial_4_bundle WIN32 
                "tac_dx12_tutorial_4_bundle.cpp"
                "tac_dx12_tutorial_4_bundle.h" )

add_executable( dx12_tutorial_5_const_buf WIN32 
                "tac_dx12_tutorial_5_const_buf.cpp"
                "tac_dx12_tutorial_5_const_buf.h" )

add_executable( dx12_tutorial_6_frame_buf WIN32 
                "tac_dx12_tutorial_6_frame_buf.cpp"
                "tac_dx12_tutorial_6_frame_buf.h" )

get_property( exe_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )

add_library( TacDX12ShaderCompile
  tac_dx12_tutorial_shader_compile.cpp
  tac_dx12_tutorial_shader_compile.h
  tac_dx12_tutorial_2_dxc.cpp
  tac_dx12_tutorial_2_dxc.h
)

target_link_libraries( TacDX12ShaderCompile TacWin32 TacRHI TacStdLib TacDX )

add_library( TacDX12ExampleHelpers
  tac_dx12_tutorial_command_queue.cpp
  tac_dx12_tutorial_command_queue.h
  tac_dx12_tutorial_gpu_upload_allocator.cpp
  tac_dx12_tutorial_gpu_upload_allocator.h
  tac_dx12_tutorial_input_layout_builder.cpp
  tac_dx12_tutorial_input_layout_builder.h
  tac_dx12_tutorial_root_sig_builder.cpp
  tac_dx12_tutorial_root_sig_builder.h
  tac_dx12_tutorial_checkerboard.cpp
  tac_dx12_tutorial_checkerboard.h
  tac_dx12_tutorial.cpp
  tac_dx12_tutorial.h
  tac_dx12_tutorial_command_allocator_pool.cpp
  tac_dx12_tutorial_command_allocator_pool.h
  tac_dx12_tutorial_context_manager.cpp
  tac_dx12_tutorial_context_manager.h
  tac_dx12_tutorial_fence.h
)

target_link_libraries( TacDX12ExampleHelpers TacWin32 TacStdLib TacDesktopApp TacDX )

foreach( targetname ${exe_targets} )
  target_link_libraries( ${targetname}
    TacStdLib
    TacEngineCore
    # TacECS
    TacDesktopApp
    TacWin32
    TacWin32Main
    TacDX
    TacDX12ShaderCompile
    TacDX12ExampleHelpers
  )
endforeach()

get_property( all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS )
foreach( targetname ${all_targets} )
  set_target_properties( ${targetname} PROPERTIES FOLDER "Programs/D3D12 Tutorial" )
endforeach()

