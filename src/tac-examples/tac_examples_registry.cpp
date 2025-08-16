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
#include "tac-examples/LaTeX_radiosity/tac_example_LaTeX_radiosity.h"
#include "tac-examples/LaTeX_simple/tac_example_LaTeX_simple.h"

#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{
  struct ExampleEntry
  {
    using ExampleFactory = Example * ( * )( );
    auto GetName() const -> const char* { return mExampleName; }
    const char*    mExampleName;
    ExampleFactory mExampleFactory;
  };

  static Vector< ExampleEntry >    sExamples;

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

} // namespace Tac

bool Tac::ExampleIndexValid( int i )
{
  return i >= 0 && i < sExamples.size();
}
auto Tac::GetExampleCount() -> int { return sExamples.size(); }
auto Tac::GetExampleName( int i ) -> const char* { return sExamples[ i ].GetName(); }
auto Tac::CreateExample( int i ) -> Tac::Example* { return sExamples[ i ].mExampleFactory(); }
void Tac::ExampleRegistryPopulate()
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
  ExampleRegistryAdd< ExampleLaTeXRadiosity >( "LaTeX Radiosity" );
  ExampleRegistryAdd< ExampleLaTeXSimple >( "LaTeX Simple" );
}
auto Tac::GetExampleIndex( const StringView& name ) -> int
{
  for( int i{}; i < sExamples.size(); ++i )
    if( ( StringView )sExamples[ i ].mExampleName == name )
      return i;
  return -1;
  }
  
