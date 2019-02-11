#pragma once

#include "common/tacString.h"

struct TacDesktopWindow;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacErrors;
struct TacShell;

struct TacCreationPropertyWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacShell* mShell = nullptr;
};

const TacString gPropertyWindowName = "PropertyWindow";
