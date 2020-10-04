#pragma once
#include "src/common/tacShell.h"
namespace Tac
{
  struct Game //: public UpdateThing
  {
    static Game* Instance;
    void Init( Errors& errors );// override;
    void SetImGuiGlobals();
    void Update( Errors& errors );// override;

    UI2DDrawData* mUi2DDrawData;
    DesktopWindowHandle mDesktopWindowHandle;
  };
}

