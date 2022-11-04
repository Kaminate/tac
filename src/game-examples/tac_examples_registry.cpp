#include "src/game-examples/tac_example_fluid.h"
#include "src/game-examples/tac_example_meta.h"
#include "src/game-examples/tac_example_phys_sim_1_force.h"
#include "src/game-examples/tac_example_phys_sim_2_integration.h"
#include "src/game-examples/tac_example_phys_sim_3_torque.h"
#include "src/game-examples/tac_example_phys_sim_4_tank.h"
#include "src/game-examples/tac_example_phys_sim_5_lincollision.h"
#include "src/game-examples/tac_example_phys_sim_6_rotcollision.h"
#include "src/game-examples/tac_example_phys_sim_7_friction.h"
#include "src/game-examples/tac_examples.h"
#include "src/common/tac_memory.h"

namespace Tac
{
#define TAC_EXAMPLE_FACTORY( T ) []()->Example*{ return TAC_NEW T; }
  void ExampleRegistryPopulate()
  {
    ExampleRegistryAdd( "Fluid", TAC_EXAMPLE_FACTORY( ExampleFluid ) );
    ExampleRegistryAdd( "Meta", TAC_EXAMPLE_FACTORY( ExampleMeta ) );
    ExampleRegistryAdd( "Physics - Sim 1 Force", TAC_EXAMPLE_FACTORY( ExamplePhysSim1Force ) );
    ExampleRegistryAdd( "Physics - Sim 2 Integration", TAC_EXAMPLE_FACTORY( ExamplePhysSim2Integration ) );
    ExampleRegistryAdd( "Physics - Sim 3 Torque", TAC_EXAMPLE_FACTORY( ExamplePhysSim3Torque ) );
    ExampleRegistryAdd( "Physics - Sim 4 Tank", TAC_EXAMPLE_FACTORY( ExamplePhysSim4Tank ) );
    ExampleRegistryAdd( "Physics - Sim 5 Lin Collision", TAC_EXAMPLE_FACTORY( ExamplePhysSim5LinCollision ) );
    ExampleRegistryAdd( "Physics - Sim 6 Rot Collision", TAC_EXAMPLE_FACTORY( ExamplePhysSim6RotCollision ) );
    ExampleRegistryAdd( "Physics - Sim 7 Friction", TAC_EXAMPLE_FACTORY( ExamplePhysSim7Friction ) );
  }


} // namespace Tac
