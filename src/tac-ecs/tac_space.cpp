#include "tac_space.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"

namespace Tac
{
  static bool sInitialized;

  void SpaceInit()
  {
    if( sInitialized )
      return;

    Graphics::SpaceInitGraphics();
    Physics::SpaceInitPhysics();

    for( const SystemInfo& entry : SystemInfo::Iterate() )
    {
      TAC_ASSERT( entry.mName );
    }

    for( const ComponentInfo& entry : ComponentInfo::Iterate() )
    {
      TAC_ASSERT( entry.mName );
    }

    sInitialized = true;
  }
}

