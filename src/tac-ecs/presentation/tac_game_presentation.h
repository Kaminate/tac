#pragma once

namespace Tac::Render
{
  struct BlendStateHandle;
  struct DepthStateHandle;
  struct RasterizerStateHandle;
  struct SamplerStateHandle;
  struct VertexDeclarations;
  struct VertexFormatHandle;
  struct ViewHandle;
}

namespace Tac
{
  struct Errors;
  struct World;
  struct Camera;
  struct Mesh;
  struct Model;
  struct Graphics;
}

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

