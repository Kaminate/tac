#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/tac_core.h"

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
