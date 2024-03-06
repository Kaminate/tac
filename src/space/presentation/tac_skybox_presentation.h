#pragma once

#include "src/common/graphics/renderer/tac_renderer.h"
#include "src/common/tac_core.h"

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
