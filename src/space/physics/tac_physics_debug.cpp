#include "space/physics/tac_physics.h"
#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-std-lib/string/tac_string.h"

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

