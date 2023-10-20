#include "src/game-examples/fluid/tac_example_fluid.h"
#include "src/game-examples/meta/tac_example_meta.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_1_force.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_2_integration.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_3_torque.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_4_tank.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_5_lincollision.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_6_rotcollision.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_7_friction.h"
#include "src/game-examples/tac_examples.h"
#include "src/game-examples/tac_examples_registry.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/containers/tac_vector.h"

namespace Tac
{

#define TAC_EXAMPLE_FACTORY( T ) []()->Example*{ return TAC_NEW T; }

  struct ExampleEntry
  {
    const char* GetName() const { return mExampleName; }
    const char* mExampleName;
    Example*( * mExampleFactory )(); 
  };


  static Vector< ExampleEntry >    sExamples;

  int GetExampleCount() { return sExamples.size(); }
  const char* GetExampleName(int i) { return sExamples[i].GetName(); }

  Example* CreateExample(int i)
  {
    return sExamples[i].mExampleFactory();
  }

  //struct ExampleIterator
  //{
  //  const ExampleEntry* begin() const;
  //  const ExampleEntry* end() const;
  //};

  //const ExampleEntry* ExampleIterator::begin() const
  //{
  //  return sExamples.begin();
  //}

  //const ExampleEntry* ExampleIterator::end() const
  //{
  //  return sExamples.end();
  //}

  static void ExampleRegistryAdd( const char* exampleName, Example* ( *exampleFactory )( ) )
  {
    // Validation
    TAC_ASSERT( !StringView( exampleName ).empty() );
    for( const ExampleEntry& example : sExamples )
      TAC_ASSERT( !StringView( example.GetName() ).contains( exampleName ) );

    ExampleEntry exampleEntry;
    exampleEntry.mExampleName = exampleName;
    exampleEntry.mExampleFactory = exampleFactory;
    sExamples.push_back( exampleEntry );
  }

  int GetExampleIndex( StringView name )
  {
    for( int i = 0; i < sExamples.size(); ++i )
      if( sExamples[ i ].mExampleName == name )
        return i;
    return -1;
  }

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

  bool ExampleIndexValid( int i )
  {
    return i >= 0 && i < sExamples.size();
  }


} // namespace Tac
