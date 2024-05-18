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
                                                        int viewWidth,
                                                        int viewHeight,
                                                        WindowHandle );
  void                          GamePresentationDebugImGui( Graphics* );
  const Mesh*                   GamePresentationGetModelMesh( const Model* );
  //Render::BufferHandle  GamePresentationGetPerFrame();
  //Render::BufferHandle  GamePresentationGetPerObj();
  Render::DepthStateHandle      GamePresentationGetDepthState();
  Render::BlendStateHandle      GamePresentationGetBlendState();
  Render::RasterizerStateHandle GamePresentationGetRasterizerState();
  Render::SamplerStateHandle    GamePresentationGetSamplerState();
  Render::VertexDeclarations    GamePresentationGetVertexDeclarations();
  Render::VertexFormatHandle    GamePresentationGetVertexFormat();
}

#endif
