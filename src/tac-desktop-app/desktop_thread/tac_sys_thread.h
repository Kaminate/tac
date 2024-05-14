// The platform thread is responsible for pumping the message queue.
// As such it is responsible for:
//
// - input devices (mouse, keyboard, controller)
// - window management (creation, destruction, movement)
// - executing render commands and swap chain present (d3d12)

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_render_state.h"

#pragma once

namespace Tac
{
  struct SysWindowApi;
  struct SysKeyboardApi;

  struct SysThread
  {
    void Init( Errors& );
    void Update( Errors& );
    void Uninit();

    App*              mApp{};
    Errors*           mErrors{};
    GameStateManager* mGameStateManager{};
    SysWindowApi*     mWindowApi{};
    SysKeyboardApi*   mKeyboardApi{};
  };
}
