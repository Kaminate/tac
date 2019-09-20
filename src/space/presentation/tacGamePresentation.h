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

  TacCamera* mCamera = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacWorld* mWorld = nullptr;

  TacSkyboxPresentation* mSkyboxPresentation = nullptr;

  // Renderer resources
  TacShader* m3DShader = nullptr;
  TacShader* mTerrainShader = nullptr;
  TacVertexFormat* m3DVertexFormat = nullptr;
  TacVertexFormat* mTerrainVertexFormat = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacCBuffer* mPerObj = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;

private:
  void Create3DShader( TacErrors& errors );
  void CreateTerrainShader( TacErrors& errors );
  void Create3DVertexFormat( TacErrors& errors );
  void CreateTerrainVertexFormat( TacErrors& errors );
  void CreatePerFrame( TacErrors& errors );
  void CreatePerObj( TacErrors& errors );
  void CreateDepthState( TacErrors& errors );
  void CreateBlendState( TacErrors& errors );
  void CreateRasterizerState( TacErrors& errors );
  void CreateSamplerState( TacErrors& errors );
};
