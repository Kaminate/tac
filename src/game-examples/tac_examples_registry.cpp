#include "src/game-examples/tac_example_fluid.h"
#include "src/game-examples/tac_example_meta.h"
#include "src/game-examples/tac_example_phys_sim_1_force.h"
#include "src/game-examples/tac_examples.h"
#include "src/common/tac_memory.h"

namespace Tac
{
  #define TAC_MAKE_EXAMPLE_FACTORY( T ) []()->Example*{ return TAC_NEW T; }
  void ExampleRegistryPopulate()
  {
    ExampleRegistryAdd( "Fluid", TAC_MAKE_EXAMPLE_FACTORY(ExampleFluid) );
    ExampleRegistryAdd( "Meta", TAC_MAKE_EXAMPLE_FACTORY(ExampleMeta) );
    ExampleRegistryAdd( "Physics - Sim 1 Force", TAC_MAKE_EXAMPLE_FACTORY(ExamplePhysSim1Force) );
  }


} // namespace Tac
