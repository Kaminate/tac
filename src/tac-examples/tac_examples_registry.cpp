#include "tac_examples_registry.h" // self-inc

#include "tac-examples/tac_examples.h"
#include "tac-examples/fluid/tac_example_fluid.h"
#include "tac-examples/meta/tac_example_meta.h"
#include "tac-examples/text/tac_example_text.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_1_force.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_2_integration.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_3_torque.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_4_tank.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_5_lincollision.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_6_rotcollision.h"
#include "tac-examples/phy_sim/tac_example_phys_sim_7_friction.h"
#include "tac-examples/imgui/tac_example_imgui.h"

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/containers/tac_vector.h"

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

  Example* CreateExample( int i )
  {
    return sExamples[ i ].mExampleFactory();
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
      .mExampleName    { exampleName },
      .mExampleFactory { []()->Example* { return TAC_NEW T; } },
    };
    sExamples.push_back( exampleEntry );
  }

  int GetExampleIndex( const StringView& name )
  {
    for( int i {}; i < sExamples.size(); ++i )
      if( ( StringView )sExamples[ i ].mExampleName == name )
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
  
