#include "tac_level_editor_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

namespace Tac
{
  struct SelectionTool : public Tool
  {
    SelectionTool()
    {
      mDisplayName = "Selection";
      mIcon = "assets/editor/tools/select_tool.png";
    }
    void OnToolSelected() override
    {
    }
    void Update() override
    {
    }
  };

  struct CubeTool : public Tool
  {
    CubeTool()
    {
      mDisplayName = "Cube";
      mIcon = "assets/editor/tools/cube_tool.png";
    }
    void OnToolSelected() override
    {
    }
    void Update() override
    {
    }
  };

  static Vector< Tool* > sRegisteredTools {};
  static Tool*           sActiveTool      {};

  void Toolbox::Init()
  {
      //sRegisteredTools.push_back( &sSelectTool );
      //sRegisteredTools.push_back( &sCubeTool );
      //SelectTool( &sSelectTool );
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
        ImGuiSameLine();
        ImGuiText( "<-- active" );
      }
    }
  }

}

