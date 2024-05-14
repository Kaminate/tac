#pragma once

#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_render_state.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
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
    SettingsRoot*     mSettingsRoot{};
  };

}
