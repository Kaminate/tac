#pragma once

struct TacBlendState;
struct TacCBuffer;
struct TacCamera;
struct TacDepthState;
struct TacDesktopWindow;
struct TacErrors;
struct TacModelAssetManager;
struct TacRasterizerState;
struct TacRenderer;
struct TacSamplerState;
struct TacShader;
struct TacString;
struct TacTextureAssetManager;
struct TacVertexFormat;

struct TacSkyboxPresentation
{
  ~TacSkyboxPresentation();
  void RenderSkybox( const TacString& skyboxDir );
  void Init( TacErrors& errors );

  TacCamera* mCamera = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;

  TacVertexFormat* mVertexFormat = nullptr;
  TacShader* mShader = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacDepthState* mDepthState  = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;
};
