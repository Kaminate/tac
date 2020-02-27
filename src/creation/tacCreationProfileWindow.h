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

struct TacCreationProfileWindow
{
  ~TacCreationProfileWindow();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void ImGui();
  void ImGuiProfile();

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  
  TacCreation* mCreation = nullptr;
};

const TacString gProfileWindowName = "ProfileWindow";
