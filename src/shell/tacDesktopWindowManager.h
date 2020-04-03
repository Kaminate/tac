#pragma once

//#include "common/tacUtility.h"
//#include "common/tacShell.h"
//#include "common/tacErrorHandling.h"
//#include "common/tacDesktopWindow.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"

struct WindowCreationData
{
  TacString mWindowName;
  int mWidth;
  int mHeight;
  int mX;
  int mY;
};
struct TacDesktopWindowManager
{
  TacDesktopWindowManager();
  static TacDesktopWindowManager* Instance;
  void SetWindowCreationData(
    const TacStringView& windowName,
    int width,
    int height,
    int x,
    int y );
  WindowCreationData* FindWindowCreationData( const TacStringView& windowName );
  TacVector< WindowCreationData > mWindowCreationData;
};

