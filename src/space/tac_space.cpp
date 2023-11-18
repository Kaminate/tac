#include "src/space/tac_space.h" // self-inc

#include "src/space/graphics/tac_graphics.h"
#include "src/space/physics/tac_physics.h"
#include "src/space/tac_component.h"

namespace Tac
{
  void SpaceInit()
  {
    Graphics::SpaceInitGraphics();
    Physics::SpaceInitPhysics();

    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      TAC_ASSERT( entry.mName );

    for( const ComponentRegistryEntry& entry : ComponentRegistryIterator() )
      TAC_ASSERT( entry.mName );
  }
}

