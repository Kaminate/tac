#include "tac_radiosity_bake_presentation.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

namespace Tac
{
  static bool sInitialized;

  // -----------------------------------------------------------------------------------------------

  static void BakeRadiosity()
  {
    // first i need to subdivide each mesh
    // maybe i can do that by having a maximum triangle side length
    // and sort all triangles by the length of their longest side.
    // then to subdivide, i add vertex in the middle of that longest side 
    // ^ wrong bitch, go into blender, right click, subdivide




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
