#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/tac_core.h"
#include "space/tac_space.h"

namespace Tac
{

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

