#pragma once

#include "common/tacErrorHandling.h"

struct TacUILayout;
struct TacShell;
struct TacUIText;
struct TacDesktopWindow;
struct TacDesktopApp;
struct TacCreation;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacUIHierarchyNode;
struct TacTexture;
struct TacCreationMainWindow;

struct TacCreationGameObjectMenuWindow
{
  ~TacCreationGameObjectMenuWindow();
  void Init( TacErrors& errors );
  void CreateLayouts();
  void Update( TacErrors& errors );

  TacCreation* mCreation = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  double mCreationSeconds;
};

