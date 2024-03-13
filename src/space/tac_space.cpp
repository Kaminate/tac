#include "tac_space.h" // self-inc

#include "space/graphics/tac_graphics.h"
#include "space/physics/tac_physics.h"
#include "space/ecs/tac_component.h"
#include "space/ecs/tac_component_registry.h"

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

