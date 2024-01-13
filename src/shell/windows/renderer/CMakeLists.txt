target_sources(TacWin PRIVATE
  "tac_dx.cpp"
  "tac_dx.h"
)

add_subdirectory(dx11)
add_subdirectory(dx12)
add_subdirectory(dxgi)
add_subdirectory(pix)
add_subdirectory(dx_shader_preprocess)

