#include "tac_level_editor_cube_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

namespace Tac
{
  CubeTool CubeTool::sInstance;
  CubeTool::CubeTool()
  {
    mDisplayName = "Cube";
    mIcon = "assets/editor/tools/cube_tool.png";
  }
  void CubeTool::OnToolSelected()
  {
  }
  void CubeTool::Update()
  {
  }

}

