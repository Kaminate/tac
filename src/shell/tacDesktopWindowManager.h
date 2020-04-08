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
    void SetWindowParams( WindowParams );
    void DoWindow( const StringView& windowName );
    void Update( Errors& errors );
    WindowParams* FindWindowParams( const StringView& windowName );
  private:
    Vector< WindowParams > mWindowParams;
    Vector< String > mWantSpawnWindows;
  };

}
