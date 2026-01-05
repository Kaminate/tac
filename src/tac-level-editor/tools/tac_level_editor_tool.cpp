#include "tac_level_editor_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-level-editor/tools/tac_level_editor_selection_tool.h"
#include "tac-level-editor/tools/tac_level_editor_cube_tool.h"
#include "tac-level-editor/tools/tac_level_editor_numgrid_tool.h"

namespace Tac
{
  static Vector< Tool* > sRegisteredTools {};
  static Tool*           sActiveTool      {};

  void Toolbox::Init()
  {
    sRegisteredTools.push_back( &SelectionTool::sInstance );
    sRegisteredTools.push_back( &CubeTool::sInstance );
    sRegisteredTools.push_back( &NumGridTool::sInstance );
    SelectTool( &SelectionTool::sInstance );
  }

  void Toolbox::SelectTool( Tool* tool )
  {
    TAC_ASSERT( Contains( sRegisteredTools, tool ) );
    if( sActiveTool != tool )
    {
      sActiveTool = tool;
      sActiveTool->OnToolSelected();
    }
  }

  auto Toolbox::GetActiveTool() -> Tool* { return sActiveTool; }

  void Toolbox::DebugImGui(Errors& errors)
  {
    for( Tool* tool : sRegisteredTools )
    {
      const bool isActive{ tool == sActiveTool };
      Render::TextureHandle textureHandle{ TextureAssetManager::GetTexture( tool->mIcon, errors ) };
      float toolIconSize{ 20.0f };

      ImGuiImage( textureHandle.GetIndex(), v2( 1, 1 ) * toolIconSize );
      ImGuiSameLine();
      if( ImGuiSelectable( tool->mDisplayName, isActive ) )
      {
        SelectTool( tool );
      }
      if( isActive )
      {
        sActiveTool->ToolUI();
      }
    }
  }

}

