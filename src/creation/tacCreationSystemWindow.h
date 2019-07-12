#pragma once

#include "common/tacString.h"

struct TacCreation;
struct TacDesktopWindow;
struct TacEntity;
struct TacErrors;
struct TacShell;
struct TacUI2DDrawData;
struct TacUIHierarchyNode;
struct TacUIRoot;

struct TacCreationSystemWindow
{
  ~TacCreationSystemWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacShell* mShell = nullptr;
  TacCreation* mCreation = nullptr;
};

const TacString gSystemWindowName = "SystemWindow";
