#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Camera;
  struct Mesh;
  struct Model;

  void                          GamePresentationInit( Errors& );
  void                          GamePresentationUninit();
  void                          GamePresentationRender( World*,
                                                        const Camera*,
                                                        int viewWidth,
                                                        int viewHeight,
                                                        Render::ViewHandle );
  const Mesh*                   GamePresentationGetModelMesh( const Model* );
}

