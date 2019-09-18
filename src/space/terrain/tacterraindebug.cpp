#include "space/terrain/tacterrain.h"
#include "common/graphics/tacImGui.h"
#include "common/tacOS.h"

void TacTerrainDebugImgui( TacTerrain* terrain )
{
  bool changed = false;
  bool changedHeightmap = false;
  if( TacImGuiCollapsingHeader( "Heightmap" ) )
  {
    if( TacImGuiInputText( "Heightmap path", terrain->mHeightmapTexturePath ) )
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
        terrain->mHeightmapTexturePath = heightmapPath;
        changedHeightmap = true;
      }
    }
    
    if( changedHeightmap )
    {
      terrain->mTestHeightmapImageMemory.clear();
      terrain->mTestHeightmapLoadErrors.clear();
      terrain->LoadTestHeightmap();
      changed |= terrain->mTestHeightmapLoadErrors.empty();
    }
  }



  if( terrain->mTestHeightmapLoadErrors.size() )
  {
    TacImGuiText( "Load heightmap errors: " + terrain->mTestHeightmapLoadErrors.ToString() );
  }
  changed |= TacImGuiDragInt( "Subdivisionness", &terrain->mSideVertexCount );
  changed |= TacImGuiDragFloat( "Size", &terrain->mSideLength );
  changed |= TacImGuiDragFloat( "Height", &terrain->mHeight );
  changed |= TacImGuiDragFloat( "Power", &terrain->mPower );
  if( changed )
  {
    terrain->mGrid.clear();
  }
}

void TacTerrainDebugImgui( TacComponent* component )
{
  TacTerrainDebugImgui( ( TacTerrain* )component );
}

