
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

struct CreationProfileWindow
{
  ~CreationProfileWindow();
  void Init( Errors& errors );
  void Update( Errors& errors );
  void ImGui();
  void ImGuiProfile();

  DesktopWindowState mDesktopWindowState;
  DesktopWindow* mDesktopWindow = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  
  Creation* mCreation = nullptr;
};

const String gProfileWindowName = "ProfileWindow";

}

