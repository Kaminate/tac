#include "tac_level_editor_numgrid_tool.h" // self-inc

#include "tac-ecs/terrain/tac_numgrid.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
//#include "tac-ecs/graphics/material/tac_material.h"
//#include "tac-ecs/graphics/model/tac_model.h"
//#include "tac-ecs/renderpass/mesh/tac_mesh_presentation.h"
//#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/windows/game/tac_level_editor_game_window.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"
#include "tac-level-editor/tac_level_editor.h"
//#include "tac-level-editor/gizmo/tac_level_editor_gizmo_mgr.h"

namespace Tac
{

  static int sBrushIndex{};
  static NumGrid* sNumGrid;
  static Errors sNumGridErrors;
  static bool sSelectingBrush;
  static bool sShowAll{ true };

  static [[nodiscard]] auto BrushesUI(Errors& errors) -> int
  {
    for( int r{}; r < 8; ++r )
    {
      for( int c{}; c < 8; ++c )
      {
        int i{ c + 8 * r };
        AssetPathStringView imgPath{ sNumGrid->mImages[ i ] };
        Render::TextureHandle imgHandle{ imgPath.empty()
          ? Render::TextureHandle{}
          : TextureAssetManager::GetTexture( imgPath, errors ) };
        if( ImGuiImageButton( imgHandle.GetIndex(), v2( 50, 50 ) ) )
        {
          return i;
        }
        if( c != 7 )
          ImGuiSameLine();
      }
    }
    return -1;
  }

  NumGridTool NumGridTool::sInstance;
  NumGridTool::NumGridTool()
  {
    mDisplayName = "NumGrid";
    mIcon = "assets/editor/tools/numgrid_tool.png";
  }
  void NumGridTool::OnToolSelected()
  {
  }

  void NumGridTool::ToolUI()
  {
    if( sNumGrid )
    {
      ImGuiText( "Brush: %i", sBrushIndex );

      AssetPathStringView brushImgPath{ sBrushIndex == -1
        ? AssetPathStringView( "" )
        : ( AssetPathStringView )sNumGrid->mImages[ sBrushIndex ] };
      Render::TextureHandle brushImgHandle{ brushImgPath.empty()
        ? Render::TextureHandle{}
        : TextureAssetManager::GetTexture( brushImgPath, sNumGridErrors ) };
      ImGuiImage( brushImgHandle.GetIndex(), v2( 50, 50 ) );

      ImGuiCheckbox( "Show All", &sShowAll );
      if( sShowAll )
      {
        if( int brushUI{ BrushesUI( sNumGridErrors ) }; brushUI != -1 )
          sBrushIndex = ( u8 )brushUI;
      }
      else
      {
        ImGuiCheckbox( "Select Brush", &sSelectingBrush );
        if( ImGuiBegin( "Select Brush", &sSelectingBrush, ImGuiWindowFlags_AutoResize ) )
        {
          if( int brushUI{ BrushesUI( sNumGridErrors ) }; brushUI != -1 )
          {
            sSelectingBrush = false;
            sBrushIndex = ( u8 )brushUI;
          }

          ImGuiEnd();
        }
      }
    }
    else
    {
      ImGuiText( "No grid selected!" );

      World* world{ Creation::GetWorld() };
      for( Entity* entity : world->mEntities )
        if( NumGrid * numGrid{ NumGrid::GetComponent( entity ) } )
          sNumGrid = numGrid;
    }

    if( sNumGridErrors )
      ImGuiText( sNumGridErrors.ToString() );
  }

  void NumGridTool::Update()
  {
    if( !sNumGrid )
      return;
    if( !AppWindowApi::IsHovered( CreationGameWindow::GetWindowHandle() ) )
      return;
    if( !AppKeyboardApi::IsPressed( Key::MouseLeft ) )
      return;

    Ray ray_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    NumGrid::WorldspaceCorners corners{ sNumGrid->GetWorldspaceCorners() };
    Triangle triBL{ corners.mBL, corners.mBR, corners.mTL };
    Triangle triTR{ corners.mTR, corners.mTL, corners.mBR };

    int r{ -1 };
    int c{ -1 };
    if( RayTriangle rayTriBL( ray_worldspace, triBL ); rayTriBL.mValid )
    {
      c = ( int )( rayTriBL.mU * sNumGrid->mWidth );
      r = ( int )( rayTriBL.mV * sNumGrid->mHeight );
    }
    if( RayTriangle rayTriTR( ray_worldspace, triTR ); rayTriTR.mValid )
    {
      c = ( int )( ( 1 - rayTriTR.mU ) * sNumGrid->mWidth );
      r = ( int )( ( 1 - rayTriTR.mV ) * sNumGrid->mHeight );
    }

    if( r != -1 && c != -1 )
      sNumGrid->mData[ c + r * sNumGrid->mWidth ] = (u8)sBrushIndex;


  }
} // namespace Tac

