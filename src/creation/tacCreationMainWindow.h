#pragma once
#include "common/tacEvent.h"
struct TacUILayout;
struct TacShell;
struct TacUIText;
struct TacDesktopWindow;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacErrors;
struct TacTexture;


struct TacCreationMainWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;

  TacUILayout* mTopMostBar = nullptr;
  TacUILayout* mTopMostBarLeft = nullptr;
  TacUILayout* mTopMostBarRight = nullptr;
  TacUILayout* mLayoutIconWindow = nullptr;
  TacUILayout* mLayoutIconClose = nullptr;
  TacUILayout* mLayoutIconMaximize = nullptr;
  TacUILayout* mLayoutIconMinimize = nullptr;
  TacUIText* mTitleText = nullptr;
  bool mAreLayoutsCreated = false;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacTexture* mIconWindow = nullptr;
  TacTexture* mIconClose = nullptr;
  TacTexture* mIconMaximize = nullptr;
  TacTexture* mIconMinimize = nullptr;
  bool mAreTexturesLoaded = false;
};
