#pragma once

struct TacBlendState;
struct TacCBuffer;
struct TacCamera;
struct TacDefaultCBufferPerObject;
struct TacDepthState;
struct TacDesktopWindow;
struct TacEntity;
struct TacMesh;
struct TacModelAssetManager;
struct TacRasterizerState;
struct TacRenderer;
struct TacSamplerState;
struct TacShader;
struct TacSkyboxPresentation;
struct TacVertexFormat;
struct TacWorld;
  struct TacDebug3DCommonData;

struct TacGamePresentation
{
  ~TacGamePresentation();
  void CreateGraphicsObjects( TacErrors& errors );
  void RenderGameWorldToDesktopView();
  void RenderGameWorldAddDrawCall( const TacMesh* mesh, const TacDefaultCBufferPerObject& cbuf );

  TacModelAssetManager* mModelAssetManager = nullptr;
  TacRenderer* mRenderer = nullptr;
  TacCamera* mCamera = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacWorld* mWorld = nullptr;

  TacSkyboxPresentation* mSkyboxPresentation = nullptr;
  TacDebug3DCommonData* mDebug3DCommonData = nullptr;

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
