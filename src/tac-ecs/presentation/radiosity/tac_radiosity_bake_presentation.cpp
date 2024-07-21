#include "tac_radiosity_bake_presentation.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

namespace Tac
{
  static bool sInitialized;

  // -----------------------------------------------------------------------------------------------

  static void BakeRadiosity()
  {
  }

  // -----------------------------------------------------------------------------------------------

  void RadiosityBakePresentation::Init( Errors& )
  {
    if( sInitialized )
      return;

    sInitialized = true;
  }

  void RadiosityBakePresentation::Render( Render::IContext* renderContext,
                                         const World* world,
                                         const Camera* camera,
                                         Errors& errors )
  {
  }

  void RadiosityBakePresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void RadiosityBakePresentation::DebugImGui()
  {
#if 0
    if( !ImGuiCollapsingHeader( "Radiosity Bake" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
#endif

    if( ImGuiButton( "Bake Radiosity" ) )
    {
      BakeRadiosity();
    }
  }
}

#endif
