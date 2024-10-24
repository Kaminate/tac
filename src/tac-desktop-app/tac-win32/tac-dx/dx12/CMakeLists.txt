target_sources( TacDX PRIVATE
  "tac_dx12_transition_helper.cpp"
  "tac_dx12_transition_helper.h"
  "tac_dx12_helper.cpp"
  "tac_dx12_helper.h"
  "tac_dx12_command_allocator_pool.cpp"
  "tac_dx12_command_allocator_pool.h"
  "tac_dx12_command_queue.cpp"
  "tac_dx12_command_queue.h"
  "tac_dx12_gpu_upload_allocator.cpp"
  "tac_dx12_gpu_upload_allocator.h"
  "tac_dx12_fence.h"
  "tac_renderer_dx12_ver3.cpp"
  "tac_renderer_dx12_ver3.h"
)

add_subdirectory( buffer )
add_subdirectory( context )
add_subdirectory( descriptor )
add_subdirectory( device )
add_subdirectory( pipeline )
add_subdirectory( program )
add_subdirectory( sampler )
add_subdirectory( texture )
add_subdirectory( resource )

