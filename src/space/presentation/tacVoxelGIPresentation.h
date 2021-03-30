#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  //struct WorldVoxelGIState;
  struct World;
  struct Camera;
  struct Errors;
  void               VoxelGIPresentationInit( Errors& );
  void               VoxelGIPresentationUninit();
  //WorldVoxelGIState* VoxelGIPresentationCreateState( World* );
  void               VoxelGIPresentationRender( World*,
                                                const Camera*,
                                                int viewWidth,
                                                int viewHeight,
                                                Render::ViewHandle );
}

