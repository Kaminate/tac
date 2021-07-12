#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Camera;
  struct Mesh;
  struct Model;
  struct Graphics;
  struct World;

  void                          ShadowPresentationInit( Errors& );
  void                          ShadowPresentationUninit();
  void                          ShadowPresentationRender( World* );
  void                          ShadowPresentationDebugImGui( Graphics* );
}

