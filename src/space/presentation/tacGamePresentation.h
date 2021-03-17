#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct BlendState;
  struct CBuffer;
  struct Camera;
  struct Debug3DCommonData;
  struct DefaultCBufferPerObject;
  struct DepthState;
  struct Entity;
  struct Mesh;
  struct Model;
  struct ModelAssetManager;
  struct RasterizerState;
  struct Renderer;
  struct SamplerState;
  struct Shader;
  struct SkyboxPresentation;
  struct Terrain;
  struct VertexFormat;
  struct World;


  struct GamePresentation
  {
    ~GamePresentation();
    void                          CreateGraphicsObjects( Errors& );
    void                          RenderGameWorldToDesktopView( World* world,
                                                                const Camera* camera,
                                                                int viewWidth,
                                                                int viewHeight,
                                                                Render::ViewHandle );
    void                          RenderGameWorldAddDrawCall( const Mesh*,
                                                              const DefaultCBufferPerObject&,
                                                              Render::ViewHandle );
    void                          LoadTerrain( Terrain* );
    void                          LoadModel( Model* );
    SkyboxPresentation*           mSkyboxPresentation = nullptr;
    Render::ShaderHandle          m3DShader;
    Render::ShaderHandle          mTerrainShader;
    Render::VertexFormatHandle    m3DVertexFormat;
    Render::VertexFormatHandle    mTerrainVertexFormat;
    Render::ConstantBufferHandle  mPerFrame;
    Render::ConstantBufferHandle  mPerObj;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    VertexDeclarations            m3DVertexFormatDecls;
  private:
    void                          Create3DShader( Errors& );
    void                          CreateTerrainShader( Errors& );
    void                          Create3DVertexFormat( Errors& );
    void                          CreateTerrainVertexFormat( Errors& );
    void                          CreatePerFrame( Errors& );
    void                          CreatePerObj( Errors& );
    void                          CreateDepthState( Errors& );
    void                          CreateBlendState( Errors& );
    void                          CreateRasterizerState( Errors& );
    void                          CreateSamplerState( Errors& );
    Errors                        mGetTextureErrorsGround;
    Errors                        mGetTextureErrorsNoise;
  };
}

