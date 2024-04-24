#pragma once

namespace Tac
{
  struct App;
  struct Errors;
  struct GameStateManager;
  struct SimWindowApi;
  struct SimKeyboardApi;

  struct SimThread
  {
    void Init( Errors& );
    void Update( Errors& );
    void Uninit();

    App*              mApp{};
    Errors*           mErrors{};
    GameStateManager* sGameStateManager{};
    SimWindowApi*     sWindowApi{};
    SimKeyboardApi*   sKeyboardApi{};
  };

}
