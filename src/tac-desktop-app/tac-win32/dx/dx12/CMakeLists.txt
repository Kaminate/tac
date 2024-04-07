target_sources( TacWin32 PRIVATE
  "tac_dx12_root_sig_builder.cpp"
  "tac_dx12_root_sig_builder.h"
  "tac_dx12_helper.cpp"
  "tac_dx12_helper.h"
  "tac_dx12_root_sig_bindings.cpp"
  "tac_dx12_root_sig_bindings.h"
  "tac_dx12_samplers.cpp"
  "tac_dx12_samplers.h"
  "tac_dx12_enum_helper.cpp"
  "tac_dx12_enum_helper.h"
  #"tac_dx12_shader_preprocess.cpp"
  #"tac_dx12_shader_preprocess.h"
  "tac_dx12_command_allocator_pool.cpp"
  "tac_dx12_command_allocator_pool.h"
  "tac_dx12_command_queue.cpp"
  "tac_dx12_command_queue.h"
  "tac_dx12_context_manager.cpp"
  "tac_dx12_context_manager.h"
  "tac_dx12_gpu_upload_allocator.cpp"
  "tac_dx12_gpu_upload_allocator.h"
  "tac_dx12_fence.h"
  #"tac_renderer_dx12.cpp"
  #"tac_renderer_dx12.h"
  # "tac_renderer_dx12_ver2.cpp"
  # "tac_renderer_dx12_ver2.h"
  "tac_renderer_dx12_ver3.cpp"
  "tac_renderer_dx12_ver3.h"
)

add_subdirectory( shaderprogram )
add_subdirectory( buffer )
add_subdirectory( pipeline )
add_subdirectory( descriptor )
add_subdirectory( device )

