#include "space/terrain/tacterrain.h"
#include "common/graphics/tacImGui.h"
#include "common/tacOS.h"

void TacTerrainDebugImgui( TacTerrain* terrain )
{
  bool changed = false;
  if( TacImGuiInputText( "Heightmap path", terrain->mHeightmapTexturePath ) )
  {
    terrain->mTestHeightmapImageMemory.clear();
    terrain->mTestHeightmapLoadErrors.clear();
    terrain->LoadTestHeightmap();
    changed |= terrain->mTestHeightmapLoadErrors.empty();
  }
  if( terrain->mTestHeightmapLoadErrors.size() )
  {
    TacImGuiText( "Load heightmap errors: " + terrain->mTestHeightmapLoadErrors.ToString() );
  }
  changed |= TacImGuiDragInt( "Subdivisionness", &terrain->mSideVertexCount );
  changed |= TacImGuiDragFloat( "Size", &terrain->mSideLength );
  changed |= TacImGuiDragFloat( "Height", &terrain->mHeight );
  if( changed )
  {
    terrain->mGrid.clear();
  }
}

void TacTerrainDebugImgui( TacComponent* component )
{
  TacTerrainDebugImgui( ( TacTerrain* )component );
}

