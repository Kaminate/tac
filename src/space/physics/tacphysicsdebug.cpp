#include "space/physics/tacphysics.h"
#include "common/graphics/tacImGui.h"

void TacPhysicsDebugImgui( TacPhysics* physics )
{
  TacImGuiText( "physics stuff" );
}
void TacPhysicsDebugImgui( TacSystem* system )
{
  TacPhysicsDebugImgui( ( TacPhysics* )system );
}
