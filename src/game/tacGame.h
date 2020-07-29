
#pragma once
namespace Tac
{
  struct Game
  {
    static Game* Instance;
    void Init( Errors& errors );
    void SetImGuiGlobals();
    void Update( Errors& errors );

    /*DesktopWindowState mDesktopWindowState*/;
    UI2DDrawData* mUi2DDrawData;
    DesktopWindowHandle mDesktopWindowHandle;
  };
}

