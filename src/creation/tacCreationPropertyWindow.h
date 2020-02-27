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

struct TacCreationPropertyWindow
{
  ~TacCreationPropertyWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void RecursiveEntityHierarchyElement( TacEntity* );

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  
  TacCreation* mCreation = nullptr;

  //TacUIHierarchyNode* mHierarchyList = nullptr;
  //TacUIHierarchyNode* mHierarchyPane = nullptr;
  //TacUIHierarchyNode* mInspector = nullptr;
};

const TacString gPropertyWindowName = "PropertyWindow";
