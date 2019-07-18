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

// you sohuld be able to open this window from the main window menu bar
// and then u can see each system ( graphics, physics, etc ) of creation->world->systems
struct TacCreationSystemWindow
{
  ~TacCreationSystemWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void ImGui();

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacShell* mShell = nullptr;
  TacCreation* mCreation = nullptr;

  int mSystemIndex = -1;
};

const TacString gSystemWindowName = "SystemWindow";
