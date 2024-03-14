target_sources( TacECS
  PUBLIC
  "tac_gjk.h"
  "tac_physics.h"
  PRIVATE
  "tac_gjk.cpp"
  "tac_physics.cpp"
  "tac_barycentric.cpp"
  "tac_barycentric.h"
  "tac_meta_physics.cpp"
  "tac_physics_debug.cpp"
)

add_subdirectory( collider )
