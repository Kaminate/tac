#include "tac-std-lib/assetmanagers/tac_model_asset_manager.h"
#include "tac-std-lib/assetmanagers/tac_mesh.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/debug/tac_debug_3d.h"
#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "space/graphics/tac_graphics.h"
#include "space/graphics/model/tac_model.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_system.h"
#include "space/ecs/tac_entity.h"
#include "space/presentation/tac_game_presentation.h"
#include "space/world/tac_world.h"
#include "space/presentation/tac_voxel_gi_presentation.h"

namespace Tac
{


  void GraphicsDebugImgui( System* system )
  {
    auto graphics = ( Graphics* )system;

    GamePresentationDebugImGui(graphics);

    VoxelGIDebugImgui();

  }
}

