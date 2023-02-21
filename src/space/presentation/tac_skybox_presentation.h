#pragma once

#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_common.h"

namespace Tac
{

  void SkyboxPresentationInit( Errors& );
  void SkyboxPresentationUninit();
  void SkyboxPresentationRender( const Camera*,
                                 int viewWidth,
                                 int viewHeight,
                                 Render::ViewHandle viewId,
                                 StringView skyboxDir );

}
