#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/tac_space.h"

namespace Tac { struct Camera; }

#define TAC_VOXEL_GI_PRESENTATION_ENABLED() 0


#if TAC_VOXEL_GI_PRESENTATION_ENABLED

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

#endif
