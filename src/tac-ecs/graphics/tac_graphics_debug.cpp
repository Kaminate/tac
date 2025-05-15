#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/game/tac_game_presentation.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/presentation/voxel/tac_voxel_gi_presentation.h"

void Tac::GraphicsDebugImgui( System* system )
{
  Graphics* graphics { ( Graphics* )system };

#if TAC_GAME_PRESENTATION_ENABLED()
  GamePresentation::DebugImGui( graphics );
#endif

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
  VoxelGIDebugImgui();
#endif
}

