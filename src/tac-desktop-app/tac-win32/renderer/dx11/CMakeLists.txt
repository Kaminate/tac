target_sources( TacWin32 PRIVATE
  "tac_renderer_dx11.cpp"
  "tac_renderer_dx11.h"
  "tac_dx11_enum_helper.cpp"
  "tac_dx11_enum_helper.h"
  "tac_dx11_namer.cpp"
  "tac_dx11_namer.h"
)

add_subdirectory( shader )
