#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-ecs/tac_space.h"

namespace Tac { struct Camera; }
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

