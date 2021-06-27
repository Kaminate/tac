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
  Render::ConstantBufferHandle  GamePresentationGetPerFrame();
  Render::ConstantBufferHandle  GamePresentationGetPerObj();
  Render::DepthStateHandle      GamePresentationGetDepthState();
  Render::BlendStateHandle      GamePresentationGetBlendState();
  Render::RasterizerStateHandle GamePresentationGetRasterizerState();
  Render::SamplerStateHandle    GamePresentationGetSamplerState();
  Render::VertexDeclarations    GamePresentationGetVertexDeclarations();
  bool&                         GamePresentationGetRenderEnabledModel();
  bool&                         GamePresentationGetRenderEnabledSkybox();
  bool&                         GamePresentationGetRenderEnabledTerrain();
  bool&                         GamePresentationGetRenderEnabledDebug3D();
}

