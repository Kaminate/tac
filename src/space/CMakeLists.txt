add_library(TacGhost
  "tac_space.cpp"
  "tac_space.h"
  "tac_space_types.h"
)

add_subdirectory(ecs)
add_subdirectory(ghost)
add_subdirectory(graphics)
add_subdirectory(net)
add_subdirectory(physics)
add_subdirectory(player)
add_subdirectory(presentation)
add_subdirectory(script)
add_subdirectory(scripts)
add_subdirectory(terrain)
add_subdirectory(world)

target_precompile_headers( TacGhost PRIVATE 
  "${CMAKE_SOURCE_DIR}/tac-std-lib/tac_precompiled_header.h"
)

# hmm.. i think this makes it so files in ghost can include 
# one another by #include "space/..."
target_include_directories(TacGhost
  PUBLIC
  "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries( TacGhost TacEngineCore )



