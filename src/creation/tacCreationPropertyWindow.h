#pragma once

#include "common/tacString.h"


struct TacDesktopWindow;
struct TacErrors;
struct TacShell;
struct TacUI2DDrawData;
struct TacUIHierarchyNode;
struct TacUIRoot;
struct TacCreation;

struct TacCreationPropertyWindow
{
  ~TacCreationPropertyWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void SetImGuiGlobals();

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacShell* mShell = nullptr;
  TacCreation* mCreation = nullptr;

  //TacUIHierarchyNode* mHierarchyList = nullptr;
  //TacUIHierarchyNode* mHierarchyPane = nullptr;
  //TacUIHierarchyNode* mInspector = nullptr;
};

const TacString gPropertyWindowName = "PropertyWindow";
