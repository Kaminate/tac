
#include "src/space/tacSpace.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/physics/tacPhysics.h"
#include "src/space/taccomponent.h"

namespace Tac
{

  void SpaceInit()
  {
    RegisterGraphicsSystem();
    Physics::SpaceInitPhysics();

    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      TAC_ASSERT( entry.mName );

    for( const ComponentRegistryEntry& entry : ComponentRegistryIterator() )
      TAC_ASSERT( entry.mName );
  }

}

