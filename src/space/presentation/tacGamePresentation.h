#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{

  void                          GamePresentationInit( Errors& );
  void                          GamePresentationUninit();
  void                          GamePresentationRender( World*,
                                                        const Camera*,
                                                        int viewWidth,
                                                        int viewHeight,
                                                        Render::ViewHandle );
}

