
#pragma once

#include "src/common/tacErrorHandling.h"

namespace Tac
{
struct UILayout;
struct Shell;
struct UIText;
struct DesktopWindow;
struct DesktopApp;
struct Creation;
struct UIRoot;
struct UI2DDrawData;
struct UIHierarchyNode;
struct Texture;
struct CreationMainWindow;

struct CreationGameObjectMenuWindow
{
  ~CreationGameObjectMenuWindow();
  void Init( Errors& errors );
  void CreateLayouts();
  void Update( Errors& errors );

  Creation* mCreation = nullptr;
  CreationMainWindow* mMainWindow = nullptr;
  DesktopWindow* mDesktopWindow = nullptr;
  UIRoot* mUIRoot = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  double mCreationSeconds;
};


}

