#pragma once

#include "tac-std-lib/input/tac_keyboard_input.h"
#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/system/tac_desktop_window.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-std-lib/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-std-lib/shell/tac_shell_timer.h" // Timepoint

namespace Tac
{
  struct App
  {
    struct IState
    {
      virtual ~IState() = default;

      FrameIndex mFrameIndex{};
      Timestamp  mTimestamp{};
      Timepoint  mTimepoint{};

    };

    struct Config
    {
      String mName;
      String mStudioName = "Sleeping Studio";

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer = false;
    };

    struct RenderParams
    {
      IState* mOldState;
      IState* mNewState;
      float mT; // [0-1]
    };

    App(const Config& config ) : mConfig( config ) {}
    virtual ~App() {};

    virtual void Init( Errors& ) {};
    virtual void Update( Errors& ) {};
    virtual void Uninit( Errors& ) {};


    virtual void Render( RenderParams, Errors& ) {};
    virtual IState* GetGameState() { return nullptr; }

    static App*  Create();
    bool         IsRenderEnabled() const { return !mConfig.mDisableRenderer; }

    Config mConfig;
  };
} // namespace Tac

