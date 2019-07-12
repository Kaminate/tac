#include "space/terrain/tacterrain.h"
#include "common/graphics/tacImGui.h"

void TacTerrainDebugImgui( TacTerrain* terrain )
{
  TacImGuiText( "hi" );
}
void TacTerrainDebugImgui( TacComponent* component )
{
  TacTerrainDebugImgui( ( TacTerrain* )component );
}

