#pragma once

#include "tac-rhi/render3/tac_render_api.h"

#define TAC_GAME_PRESENTATION_ENABLED() 1

#if TAC_GAME_PRESENTATION_ENABLED()

namespace Tac
{
  struct Errors;
  struct World;
  struct Camera;
  struct Mesh;
  struct Model;
  struct Graphics;
  struct WindowHandle;
}

namespace Tac
{

  void                          GamePresentationInit( Errors& );
  void                          GamePresentationUninit();
  void                          GamePresentationRender( World*, // why is this non const?
                                                        const Camera*,
                                                        v2i viewSize,
                                                        Render::TextureHandle,
                                                        Errors& );
  void                          GamePresentationDebugImGui( Graphics* );
  const Mesh*                   GamePresentationGetModelMesh( const Model* );
}

#endif
