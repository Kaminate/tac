#pragma once
struct TacDesktopWindow;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacErrors;

struct TacCreationPropertyWindow
{
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
};
