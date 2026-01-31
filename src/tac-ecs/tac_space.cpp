#include "tac_space.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/terrain/tac_numgrid.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/component/tac_component_registry.h"

static bool sInitialized;

void Tac::SpaceInit()
{
  if( sInitialized )
    return;

  Graphics::SpaceInitGraphics();
  Physics::SpaceInitPhysics();
  NumGridSys::SpaceInitNumGrid();

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

