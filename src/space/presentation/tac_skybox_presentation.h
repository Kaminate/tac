#pragma once

#include "tac-rhi/renderer/tac_renderer.h"

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
