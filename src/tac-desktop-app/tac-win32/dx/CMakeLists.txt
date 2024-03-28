target_sources( TacWin32 PRIVATE
  "tac_dx.cpp"
  "tac_dx.h"
)

# add_subdirectory( dx11 )
add_subdirectory( dxc )
add_subdirectory( dx12 )
add_subdirectory( dxgi )
add_subdirectory( pix )
add_subdirectory( hlsl )

