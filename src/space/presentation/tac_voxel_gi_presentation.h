#pragma once

#include "src/common/tac_common.h"
#include "src/space/tac_space.h"

namespace Tac
{
  void               VoxelGIPresentationInit( Errors& );
  void               VoxelGIPresentationUninit();
  void               VoxelGIPresentationRender( const World*,
                                                const Camera*,
                                                int viewWidth,
                                                int viewHeight,
                                                Render::ViewHandle );
  void               VoxelGIDebugImgui();
}

