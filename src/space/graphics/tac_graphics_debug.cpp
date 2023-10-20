#include "src/common/assetmanagers/tac_model_asset_manager.h"
#include "src/common/assetmanagers/tac_mesh.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/string/tac_string.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/space/graphics/tac_graphics.h"
#include "src/space/model/tac_model.h"
#include "src/space/tac_component.h"
#include "src/space/tac_system.h"
#include "src/space/tac_entity.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_world.h"
#include "src/space/presentation/tac_voxel_gi_presentation.h"

namespace Tac
{


  void GraphicsDebugImgui( System* system )
  {
    auto graphics = ( Graphics* )system;

    GamePresentationDebugImGui(graphics);

    VoxelGIDebugImgui();

  }
}

