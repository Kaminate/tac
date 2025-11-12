target_sources(TacECS
  PUBLIC
  "tac_graphics.h"
  PRIVATE
  "tac_graphics_debug.cpp"
  "tac_graphics.cpp"
)

add_subdirectory( light )
add_subdirectory( model )
add_subdirectory( skybox )
add_subdirectory( material )
add_subdirectory( camera )

