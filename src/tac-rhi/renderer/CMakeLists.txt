target_sources( TacRHI PRIVATE
  tac_render_frame.cpp
  tac_render_frame.h
  tac_render_id.cpp
  tac_render_id.h
  tac_render_submit_ring_buffer.cpp
  tac_render_submit_ring_buffer.h
  tac_render_uniform_buffer.cpp
  tac_render_uniform_buffer.h
  tac_renderer.cpp
  tac_renderer.h
  tac_renderer_backend.cpp
  tac_renderer_backend.h
  tac_shader_reload_helper.cpp
  tac_shader_reload_helper.h
)

add_subdirectory( command )
