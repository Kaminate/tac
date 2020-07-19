#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{

  struct BlendState;
  struct CBuffer;
  struct Camera;
  struct DefaultCBufferPerObject;
  struct DepthState;
  struct DesktopWindow;
  struct Entity;
  struct Mesh;
  struct ModelAssetManager;
  struct RasterizerState;
  struct Renderer;
  struct SamplerState;
  struct Shader;
  struct SkyboxPresentation;
  struct VertexFormat;
  struct World;
  struct Debug3DCommonData;


  struct GamePresentation
  {
    ~GamePresentation();
    void CreateGraphicsObjects( Errors& errors );
    void RenderGameWorldToDesktopView( int viewWidth,
                                       int viewHeight,
                                       Render::ViewId viewId );
    void RenderGameWorldAddDrawCall( const Mesh* mesh,
                                     const DefaultCBufferPerObject& cbuf,
                                     Render::ViewId viewId );

    Camera* mCamera = nullptr;
    World* mWorld = nullptr;

    SkyboxPresentation* mSkyboxPresentation = nullptr;

    // Renderer resources
    Render::ShaderHandle m3DShader;
    Render::ShaderHandle mTerrainShader;
    Render::VertexFormatHandle m3DVertexFormat;
    Render::VertexFormatHandle mTerrainVertexFormat;
    Render::ConstantBufferHandle mPerFrame;
    Render::ConstantBufferHandle mPerObj;
    Render::DepthStateHandle mDepthState;
    Render::BlendStateHandle mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle mSamplerState;

    static const int k3DVertexFormatDeclCount = 1;
    VertexDeclaration m3DVertexFormatDecls[k3DVertexFormatDeclCount];

  private:
    void Create3DShader( Errors& errors );
    void CreateTerrainShader( Errors& errors );
    void Create3DVertexFormat( Errors& errors );
    void CreateTerrainVertexFormat( Errors& errors );
    void CreatePerFrame( Errors& errors );
    void CreatePerObj( Errors& errors );
    void CreateDepthState( Errors& errors );
    void CreateBlendState( Errors& errors );
    void CreateRasterizerState( Errors& errors );
    void CreateSamplerState( Errors& errors );
  };
}

