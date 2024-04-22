#pragma once

namespace Tac::Render { struct ViewHandle; }
namespace Tac { struct Errors; struct Camera; struct AssetPathStringView; }

#define TAC_SKYBOX_PRESENTATION_ENABLED() 0

#if TAC_SKYBOX_PRESENTATION_ENABLED()

namespace Tac
{
  void SkyboxPresentationInit( Errors& );
  void SkyboxPresentationUninit();
  void SkyboxPresentationRender( const Camera*,
                                 int viewWidth,
                                 int viewHeight,
                                 Render::ViewHandle viewId,
                                 const AssetPathStringView& skyboxDir );

}

#endif
