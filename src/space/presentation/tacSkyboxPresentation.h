#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Camera;
  struct Errors;

  void SkyboxPresentationInit( Errors& );
  void SkyboxPresentationUninit();
  void SkyboxPresentationRender( const Camera*,
                                 int viewWidth,
                                 int viewHeight,
                                 Render::ViewHandle viewId,
                                 StringView skyboxDir );

}
