#pragma once

#include "common/tacString.h"
#include "common/math/tacVector3.h"

struct TacEntity;
struct TacModel;
struct TacErrors;
struct TacDesktopWindow;
struct TacCreation;
struct TacUIRoot;
struct TacDebug3DDrawData;
struct TacUI2DDrawData;
struct TacSoul;
struct TacShell;
struct TacRenderView;
struct TacShader;
struct TacVertexFormat;
struct TacCBuffer;
struct TacDepthState;
struct TacBlendState;
struct TacRasterizerState;
struct TacSamplerState;
struct TacMesh;
struct CBufferPerObject;
struct TacGamePresentation;
struct TacSkyboxPresentation;

struct TacCreationGameWindow
{
  ~TacCreationGameWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void RenderGameWorld();
  void MousePickingInit();
  void MousePicking( const TacEntity* entity, bool* hit, float* dist );
  void MousePicking();
  void AddDrawCall( const TacMesh* mesh, const CBufferPerObject& cbuf );
  void SetImGuiGlobals();
  void ComputeArrowLen();
  void CreateGraphicsObjects( TacErrors& errors );
  void DrawPlaybackOverlay( TacErrors& errors );
  void PlayGame( TacErrors& errors );

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacSoul* mSoul = nullptr;
  TacCreation* mCreation = nullptr;

  // Renderer resources
  TacShader* m3DShader = nullptr;
  TacVertexFormat* m3DVertexFormat = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacCBuffer* mPerObj = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;

  TacDebug3DDrawData* mDebug3DDrawData = nullptr;

  TacGamePresentation* mGamePresentation = nullptr;
  TacSkyboxPresentation* mSkyboxPresentation = nullptr;

  TacMesh* mArrow = nullptr;
  v3 worldSpaceMouseDir = {};
  float mArrowLen = 0;
};


const TacString gGameWindowName = "VirtualGamePlayer";
