
#include "src/space/tacSpace.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/physics/tacPhysics.h"

namespace Tac
{

  void SpaceInit()
  {
    Graphics::SpaceInitGraphics();
    Physics::SpaceInitPhysics();

    for( SystemRegistryEntry& entry : SystemRegistry::Instance()->mEntries )
    {
      TAC_ASSERT( entry.mName.size() );
    }

    for( ComponentRegistryEntry& entry : ComponentRegistryIterator() )
    {
      TAC_ASSERT( entry.mName.size() );
    }
  }

}

