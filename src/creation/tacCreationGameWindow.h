#pragma once

struct TacErrors;
struct TacDesktopWindow;
struct TacCreation;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacSoul;
struct TacShell;
struct TacRenderView;

struct TacCreationGameWindow
{
  void Init( TacErrors& errors);
  void Update( TacErrors& errors );

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;

  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacSoul* mSoul = nullptr;
  TacRenderView* mRenderView = nullptr;
};

