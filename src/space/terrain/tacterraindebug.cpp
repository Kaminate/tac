#include "src/space/terrain/tacterrain.h"
#include "src/space/tacentity.h"
#include "src/space/tacworld.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacOS.h"
#include "src/common/graphics/tacDebug3D.h"

namespace Tac
{
  struct TerainDebugger
  {
    void DebugImgui()
    {
      bool changed = false;
      bool changedHeightmap = false;
      if( ImGuiCollapsingHeader( "Heightmap" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        if( ImGuiInputText( "Heightmap path", mTerrain->mHeightmapTexturePath ) )
        {
          changedHeightmap = true;
        }

        Vector< String > heightmapPaths;
        Errors errors;
        OSGetFilesInDirectory( heightmapPaths, "assets/heightmaps", OSGetFilesInDirectoryFlags::Recursive, errors );
        for( auto& heightmapPath : heightmapPaths )
        {
          if( ImGuiButton( heightmapPath ) )
          {
            mTerrain->mHeightmapTexturePath = heightmapPath;
            changedHeightmap = true;
          }
        }

        if( changedHeightmap )
        {
          mTerrain->mTestHeightmapLoadErrors.clear();
          mTerrain->mTestHeightmapImageMemory.clear();
          mTerrain->LoadTestHeightmap();
          changed |= mTerrain->mTestHeightmapLoadErrors.empty();
        }
      }

      ImGuiCheckbox( "Draw grid", &mDrawGrid );

      ImGuiInputText( "Ground texture", mTerrain->mGroundTexturePath );
      ImGuiInputText( "Noise texture", mTerrain->mNoiseTexturePath );
      if( ImGuiButton( "Open Ground Texture" ) )
      {
        mTerrain->mTestHeightmapLoadErrors.clear();
        OSOpenDialog( mTerrain->mGroundTexturePath, mTerrainTextureDialogErrors );
      }
      if( ImGuiButton( "Open Noise Texture" ) )
      {
        OSOpenDialog( mTerrain->mNoiseTexturePath, mNoiseTextureDialogErrors );
      }

      if( mTerrain->mTestHeightmapLoadErrors )
        ImGuiText( "Load heightmap errors: " + mTerrain->mTestHeightmapLoadErrors.ToString() );
      if( mTerrainTextureDialogErrors )
        ImGuiText( "Dialog heightmap errors: " + mTerrainTextureDialogErrors.ToString() );
      if( mNoiseTextureDialogErrors )
        ImGuiText( "Dialog noise texture errors: " + mNoiseTextureDialogErrors.ToString() );

      changed |= ImGuiDragInt( "Subdivisions", &mTerrain->mSideVertexCount );
      changed |= ImGuiDragFloat( "Size", &mTerrain->mSideLength );
      changed |= ImGuiDragFloat( "Height", &mTerrain->mUpwardsHeight );
      changed |= ImGuiDragFloat( "Power", &mTerrain->mPower );
      if( changed )
      {
        mTerrain->Recompute();
      }

      if( mDrawGrid )
        DebugDraw();
    }

    void DebugDraw()
    {
      Entity* entity = mTerrain->mEntity;
      World* world = entity->mWorld;
      Debug3DDrawData* debug3DDrawData = world->mDebug3DDrawData;
      v3 gridColor = { 0, 0, 0 };
      v3 offset = { 0, 0.1f, 0 }; // for zfighting against the tri mesh

      if( mTerrain->mRowMajorGrid.empty() )
        return;

      if( !mDrawGrid )
        return;

      for( int iRow = 0; iRow < mTerrain->mSideVertexCount; ++iRow )
      {
        for( int iCol = 0; iCol < mTerrain->mSideVertexCount; ++iCol )
        {
          v3 topLeft = mTerrain->GetGridVal( iRow, iCol ) + offset;

          if( iCol + 1 < mTerrain->mSideVertexCount )
          {
            v3 topRight = mTerrain->GetGridVal( iRow, iCol + 1 ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, topRight, gridColor );
          }
          if( iRow + 1 < mTerrain->mSideVertexCount )
          {
            v3 bottomLeft = mTerrain->GetGridVal( iRow + 1, iCol ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomLeft, gridColor );
          }
          if( iCol + 1 < mTerrain->mSideVertexCount && iRow + 1 < mTerrain->mSideVertexCount )
          {
            v3 bottomRight = mTerrain->GetGridVal( iRow + 1, iCol + 1 ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomRight, gridColor );
          }
        }
      }
    }

    bool mDrawGrid = false;
    Errors mNoiseTextureDialogErrors;
    Errors mTerrainTextureDialogErrors;
    Terrain* mTerrain = nullptr;
  } gTerrainDebugger;

  void TerrainDebugImgui( Terrain* terrain )
  {
    gTerrainDebugger.mTerrain = terrain;
    gTerrainDebugger.DebugImgui();
  }


}

