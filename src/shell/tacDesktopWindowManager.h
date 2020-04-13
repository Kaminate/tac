#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacDesktopWindow.h"

namespace Tac
{


  struct DesktopWindowManager
  {
    DesktopWindowManager();
    static DesktopWindowManager* Instance;
    DesktopWindowHandle CreateWindow( int x, int y, int width, int height );
    void Update( Errors& errors );
  };

}
