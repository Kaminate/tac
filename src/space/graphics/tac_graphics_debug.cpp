#include "src/common/assetmanagers/tac_model_asset_manager.h"
#include "src/common/assetmanagers/tac_mesh.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/string/tac_string.h"
#include "src/common/preprocess/tac_preprocessor.h"
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

