#include "space/physics/tac_physics.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/string/tac_string.h"

namespace Tac
{
  void PhysicsDebugImgui( Physics* physics )
  {
    TAC_UNUSED_PARAMETER( physics );
    ImGuiText( "physics stuff" );
  }

  void PhysicsDebugImgui( System* system )
  {
    PhysicsDebugImgui( ( Physics* )system );
  }

}

