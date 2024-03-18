// The platform thread is responsible for pumping the message queue.
// As such it is responsible for:
//
// - input devices (mouse, keyboard, controller)
// - window management (creation, destruction, movement)
// - executing render commands and swap chain present (d3d12)

#pragma once

namespace Tac
{
  struct Errors;
  struct App;
  struct GameStateManager;

  struct PlatformThread
  {
    void Init( Errors& );
    void Update( Errors& );
    void Uninit();

    App*    mApp = nullptr;
    Errors* mErrors = nullptr;
    GameStateManager* sGameStateManager = nullptr;
  };
}
