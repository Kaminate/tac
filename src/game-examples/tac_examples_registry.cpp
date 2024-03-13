#include "tac_examples_registry.h" // self-inc

#include "src/game-examples/tac_examples.h"
#include "src/game-examples/fluid/tac_example_fluid.h"
#include "src/game-examples/meta/tac_example_meta.h"
#include "src/game-examples/text/tac_example_text.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_1_force.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_2_integration.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_3_torque.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_4_tank.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_5_lincollision.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_6_rotcollision.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_7_friction.h"
#include "src/game-examples/imgui/tac_example_imgui.h"

#include "src/common/memory/tac_memory.h"
#include "src/common/containers/tac_vector.h"

namespace Tac
{

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


  template< typename T >
  static void ExampleRegistryAdd( const char* exampleName )
  {
    // Validation
    TAC_ASSERT( !StringView( exampleName ).empty() );
    for( const ExampleEntry& example : sExamples )
      TAC_ASSERT( !StringView( example.GetName() ).contains( exampleName ) );

    const ExampleEntry exampleEntry
    {
      .mExampleName = exampleName,
      .mExampleFactory = []()->Example* { return TAC_NEW T; },
    };
    sExamples.push_back( exampleEntry );
  }

  int GetExampleIndex( const StringView& name )
  {
    for( int i = 0; i < sExamples.size(); ++i )
      if( sExamples[ i ].mExampleName == name )
        return i;
    return -1;
  }

  void ExampleRegistryPopulate()
  {
    ExampleRegistryAdd< ExampleFluid >( "Fluid" );
    ExampleRegistryAdd< ExampleMeta >( "Meta" );
    ExampleRegistryAdd< ExamplePhysSim1Force >( "Physics - Sim 1 Force" );
    ExampleRegistryAdd< ExamplePhysSim2Integration >( "Physics - Sim 2 Integration" );
    ExampleRegistryAdd< ExamplePhysSim3Torque >( "Physics - Sim 3 Torque" );
    ExampleRegistryAdd< ExamplePhysSim4Tank >( "Physics - Sim 4 Tank" );
    ExampleRegistryAdd< ExamplePhysSim5LinCollision >( "Physics - Sim 5 Lin Collision" );
    ExampleRegistryAdd< ExamplePhysSim6RotCollision >( "Physics - Sim 6 Rot Collision" );
    ExampleRegistryAdd< ExamplePhysSim7Friction >( "Physics - Sim 7 Friction" );
    ExampleRegistryAdd< ExampleText >( "Text" );
    ExampleRegistryAdd< ExampleImgui >( "ImGui" );
  }

  bool ExampleIndexValid( int i )
  {
    return i >= 0 && i < sExamples.size();
  }


} // namespace Tac
  
