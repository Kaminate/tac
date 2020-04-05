#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{


struct WindowCreationData
{
  String mWindowName;
  int mWidth;
  int mHeight;
  int mX;
  int mY;
};
struct DesktopWindowManager
{
  DesktopWindowManager();
  static DesktopWindowManager* Instance;
  void SetWindowCreationData(
    const StringView& windowName,
    int width,
    int height,
    int x,
    int y );
  WindowCreationData* FindWindowCreationData( const StringView& windowName );
  Vector< WindowCreationData > mWindowCreationData;
};

}
