#pragma once

#include "common/tacString.h"

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

struct TacCreationGameWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void RenderGameWorld();
  void MousePicking();
  void SetImGuiGlobals();
  void CreateGraphicsObjects( TacErrors& errors);
  void DrawPlaybackOverlay( TacErrors& errors );
  void PlayGame( TacErrors& errors );

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacSoul* mSoul = nullptr;
  TacCreation* mCreation = nullptr;
  TacShader* m3DShader = nullptr;
  TacVertexFormat* m3DVertexFormat = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacCBuffer* mPerObj = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;
  TacDebug3DDrawData* mDebug3DDrawData = nullptr;
};


const TacString gGameWindowName = "VirtualGamePlayer";
