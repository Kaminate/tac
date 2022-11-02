#include "src/game-examples/tac_example_phys_sim_2_integration.h"
#include "src/common/graphics/imgui/tac_imgui.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{
  const char* ToString( IntegrationMode m )
  {
    switch( m )
    {
      case Euler: return "Euler";
      default: return "";
    }

  }

  ExamplePhysSim2Integration::ExamplePhysSim2Integration()
  {

  }

  ExamplePhysSim2Integration::~ExamplePhysSim2Integration()
  {

  }

  void ExamplePhysSim2Integration::Update( Errors& )
  {
    for( int i = 0; i < IntegrationMode::Count; ++i )
    {
      auto mode = (IntegrationMode)i;
      if( ImGuiButton( ToString( mode ) ) )
      {
        mIntegrationMode = mode;
      }
    }
  }

  const char* ExamplePhysSim2Integration::GetName() const { return "Phys Sim 2 Integration"; }

} // namespace Tac
