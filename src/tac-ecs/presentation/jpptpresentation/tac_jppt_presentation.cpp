#include "tac_jppt_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"


#if TAC_JPPT_PRESENTATION_ENABLED()

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  static bool sInitialized;
  static bool sEnabled;

  void             JPPTPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;


    sInitialized = true;
  }

  void             JPPTPresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void             JPPTPresentation::Render( Render::IContext* renderContext,
                                             const World* world,
                                             const Camera* camera,
                                             const v2i viewSize,
                                             const Render::TextureHandle dstColorTex,
                                             const Render::TextureHandle dstDepthTex,
                                             Errors& errors )
  {
    TAC_ASSERT( sInitialized );
    if( !sEnabled )
      return;


  }

  void             JPPTPresentation::DebugImGui()
  {
    ImGuiCheckbox( "JPPT Presentation Enabled", &sEnabled );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac

#endif // TAC_JPPT_PRESENTATION_ENABLED
