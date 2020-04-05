#pragma once

#include "src/common/tacString.h"

namespace Tac
{
struct Creation;
struct DesktopWindow;
struct Entity;
struct Errors;
struct Shell;
struct UI2DDrawData;
struct UIHierarchyNode;
struct UIRoot;



struct CreationPropertyWindow
{
  ~CreationPropertyWindow();
  void Init( Errors& errors );
  void Update( Errors& errors );
  void RecursiveEntityHierarchyElement( Entity* );

  DesktopWindow* mDesktopWindow = nullptr;
  UIRoot* mUIRoot = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  
  Creation* mCreation = nullptr;

  //UIHierarchyNode* mHierarchyList = nullptr;
  //UIHierarchyNode* mHierarchyPane = nullptr;
  //UIHierarchyNode* mInspector = nullptr;
};

const String gPropertyWindowName = "PropertyWindow";

}
