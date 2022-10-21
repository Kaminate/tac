#pragma once

#include "src/common/graphics/tac_renderer.h"

namespace Tac
{
  struct Camera;
  struct Mesh;
  struct Model;
  struct Graphics;
  struct World;

  void                          GamePresentationInit( Errors& );
  void                          GamePresentationUninit();
  void                          GamePresentationRender( World*, // why is this non const?
                                                        const Camera*,
                                                        int viewWidth,
                                                        int viewHeight,
                                                        Render::ViewHandle );
  void                          GamePresentationDebugImGui( Graphics* );
  const Mesh*                   GamePresentationGetModelMesh( const Model* );
  //Render::ConstantBufferHandle  GamePresentationGetPerFrame();
  //Render::ConstantBufferHandle  GamePresentationGetPerObj();
  Render::DepthStateHandle      GamePresentationGetDepthState();
  Render::BlendStateHandle      GamePresentationGetBlendState();
  Render::RasterizerStateHandle GamePresentationGetRasterizerState();
  Render::SamplerStateHandle    GamePresentationGetSamplerState();
  Render::VertexDeclarations    GamePresentationGetVertexDeclarations();
  Render::VertexFormatHandle    GamePresentationGetVertexFormat();
}

