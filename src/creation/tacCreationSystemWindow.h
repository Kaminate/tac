
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

// you sohuld be able to open this window from the main window menu bar
// and then u can see each system ( graphics, physics, etc ) of creation->world->systems
struct CreationSystemWindow
{
  ~CreationSystemWindow();
  void Init( Errors& errors );
  void Update( Errors& errors );
  void ImGui();

  DesktopWindowState mDesktopWindowState;
  DesktopWindow* mDesktopWindow = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  
  Creation* mCreation = nullptr;

  int mSystemIndex = -1;
};

const String gSystemWindowName = "SystemWindow";

}

