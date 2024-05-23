target_sources( TacWin32
PUBLIC
  "tac_dx11_shader_compiler.h"
  "tac_dx11_shader_postprocess.h"
  "tac_dx11_shader_preprocess.h"
  "tac_dx11_shader_program.h"

PRIVATE
  "tac_dx11_shader_compiler.cpp"
  "tac_dx11_shader_postprocess.cpp"
  "tac_dx11_shader_pre_bitfield.cpp"
  "tac_dx11_shader_pre_fx.cpp"
  "tac_dx11_shader_pre_includes.cpp"
  "tac_dx11_shader_pre_padding.cpp"
  "tac_dx11_shader_pre_register.cpp"
  "tac_dx11_shader_pre_semantic.cpp"
  "tac_dx11_shader_preprocess.cpp"
  "tac_dx11_shader_program.cpp"
)

