#include "src/space/physics/tacphysics.h"
#include "src/common/graphics/imgui/tacImGui.h"

namespace Tac
{
void PhysicsDebugImgui( Physics* physics )
{
  ImGuiText( "physics stuff" );
}
void PhysicsDebugImgui( System* system )
{
  PhysicsDebugImgui( ( Physics* )system );
}

}

