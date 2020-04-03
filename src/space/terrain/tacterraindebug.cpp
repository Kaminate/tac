#include "space/terrain/tacterrain.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "common/graphics/imgui/tacImGui.h"
#include "common/tacOS.h"
#include "common/graphics/tacDebug3D.h"

struct TacTerainDebugger
{
  void DebugImgui()
  {
    bool changed = false;
    bool changedHeightmap = false;
    if( TacImGuiCollapsingHeader( "Heightmap" ) )
    {
      if( TacImGuiInputText( "Heightmap path", mTerrain->mHeightmapTexturePath ) )
      {
        changedHeightmap = true;
      }

      TacOS* os = TacOS::Instance;
      TacVector< TacString > heightmapPaths;
      TacErrors errors;
      TacOS::Instance->GetDirFilesRecursive( heightmapPaths, "assets/heightmaps", errors );
      for( const TacString& heightmapPath : heightmapPaths )
      {
        if( TacImGuiButton( heightmapPath ) )
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

    TacImGuiCheckbox( "Draw grid", &mDrawGrid );

    TacImGuiInputText( "Ground texture", mTerrain->mGroundTexturePath );
    TacImGuiInputText( "Noise texture", mTerrain->mNoiseTexturePath );
    if( TacImGuiButton( "Open Ground Texture" ) )
    {
      mTerrain->mTestHeightmapLoadErrors.clear();
      TacOS::Instance->OpenDialog( mTerrain->mGroundTexturePath, mTerrainTextureDialogErrors );
    }
    if( TacImGuiButton( "Open Noise Texture" ) )
    {
      TacOS::Instance->OpenDialog( mTerrain->mNoiseTexturePath, mNoiseTextureDialogErrors );
    }

    if( mTerrain->mTestHeightmapLoadErrors )
      TacImGuiText( "Load heightmap errors: " + mTerrain->mTestHeightmapLoadErrors.ToString() );
    if( mTerrainTextureDialogErrors )
      TacImGuiText( "Dialog heightmap errors: " + mTerrainTextureDialogErrors.ToString() );
    if( mNoiseTextureDialogErrors )
      TacImGuiText( "Dialog noise texture errors: " + mNoiseTextureDialogErrors.ToString() );

    changed |= TacImGuiDragInt( "Subdivisionness", &mTerrain->mSideVertexCount );
    changed |= TacImGuiDragFloat( "Size", &mTerrain->mSideLength );
    changed |= TacImGuiDragFloat( "Height", &mTerrain->mUpwardsHeight );
    changed |= TacImGuiDragFloat( "Power", &mTerrain->mPower );
    if( changed )
    {
      mTerrain->Recompute();
    }

    if( mDrawGrid )
      DebugDraw();
  }

  void DebugDraw()
  {
    TacEntity* entity = mTerrain->mEntity;
    TacWorld* world = entity->mWorld;
    TacDebug3DDrawData* debug3DDrawData = world->mDebug3DDrawData;
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
          debug3DDrawData->DebugDrawLine( topLeft, topRight, gridColor );
        }
        if( iRow + 1 < mTerrain->mSideVertexCount )
        {
          v3 bottomLeft = mTerrain->GetGridVal( iRow + 1, iCol ) + offset;
          debug3DDrawData->DebugDrawLine( topLeft, bottomLeft, gridColor );
        }
        if( iCol + 1 < mTerrain->mSideVertexCount && iRow + 1 < mTerrain->mSideVertexCount )
        {
          v3 bottomRight = mTerrain->GetGridVal( iRow + 1, iCol + 1 ) + offset;
          debug3DDrawData->DebugDrawLine( topLeft, bottomRight, gridColor );
        }
      }
    }
  }

  bool mDrawGrid = false;
  TacErrors mNoiseTextureDialogErrors;
  TacErrors mTerrainTextureDialogErrors;
  TacTerrain* mTerrain = nullptr;
} gTerrainDebugger;

void TacTerrainDebugImgui( TacComponent* component )
{
  gTerrainDebugger.mTerrain =  ( TacTerrain* )component ;
  gTerrainDebugger.DebugImgui();
}

