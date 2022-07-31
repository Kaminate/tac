project(Ghost CXX)
set(Target "Ghost")
set( TAC_LIBRARIES CommonLibrary  )
#                        | ( commented out )
#                        |
#                        | | MODULE is for DLLs
#                        v v


# ${CMAKE_CURRENT_SOURCE_DIR}/myfile.h

add_library(${Target} # MODULE
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_voxel_gi_presentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_voxel_gi_presentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_game_presentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_game_presentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_shadow_presentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_shadow_presentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_skybox_presentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tac_skybox_presentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_client.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_client.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_component.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_component.h"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tac_collider.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tac_collider.h"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tac_collider_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_entity.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_entity.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_gjk.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_gjk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_ghost.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_ghost.h"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tac_graphics_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tac_graphics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tac_graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/skybox/tac_skybox_component.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/skybox/tac_skybox_component.h"
"${CMAKE_CURRENT_SOURCE_DIR}/skybox/tac_skybox_component_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tac_model.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tac_model.h"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tac_model_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/light/tac_light.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/light/tac_light.h"
"${CMAKE_CURRENT_SOURCE_DIR}/light/tac_light_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_player.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_player.h"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tac_meta_physics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tac_physics_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tac_physics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tac_physics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_script.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_script.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_script_game_client.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_script_game_client.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_space.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_space.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_space_types.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_system.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_system.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_server.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_server.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_space_net.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_space_net.h"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tac_terrain.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tac_terrain.h"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tac_terrain_debug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_world.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_world.h"
)
#${Common})

target_precompile_headers( ${Target} PRIVATE 
  "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h"
)

include_directories("${CMAKE_SOURCE_DIR}/src")
set_target_properties(${Target} PROPERTIES LINKER_LANGUAGE CXX)

if( WIN32 )
  add_definitions(/ZI)
endif()

target_link_libraries( ${Target} ${TAC_LIBRARIES} )



