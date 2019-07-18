#include "common/graphics/tacImGui.h"
#include "space/graphics/tacgraphics.h"
#include "space/graphics/tacgraphicsdebug.h"
void TacGraphicsDebugImgui( TacSystem* system )
{
  TacGraphicsDebugImgui( ( TacGraphics* )system );
}
void TacGraphicsDebugImgui( TacGraphics* graphics )
{
  TacImGuiText( "graphics stuff" );
}
