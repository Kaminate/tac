#pragma once

struct TacModelAssetManager;
struct TacRenderer;
struct TacCamera;
struct TacDesktopWindow;
struct TacShader;
struct TacVertexFormat;
struct TacCBuffer;
struct TacCBuffer;
struct TacDepthState;
struct TacBlendState;
struct TacRasterizerState;
struct TacSamplerState;
struct TacWorld;
struct TacMesh;
struct CBufferPerObject;
struct TacSkyboxPresentation;

struct TacGamePresentation
{
  ~TacGamePresentation();
  void RenderGameWorld();
  void CreateGraphicsObjects( TacErrors& errors );
  void AddDrawCall( const TacMesh* mesh, const CBufferPerObject& cbuf );

  TacModelAssetManager* mModelAssetManager = nullptr;
  TacRenderer* mRenderer = nullptr;
  TacCamera* mCamera = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacWorld* mWorld = nullptr;

  TacSkyboxPresentation* mSkyboxPresentation = nullptr;

  // Renderer resources
  TacShader* m3DShader = nullptr;
  TacVertexFormat* m3DVertexFormat = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacCBuffer* mPerObj = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;
};
