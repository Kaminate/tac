#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_core.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/containers/tac_list.h"
#include "src/common/shell/tac_shell_timestep.h" // FrameIndex
#include "src/common/shell/tac_shell_timestamp.h" // Timestamp
#include "src/common/shell/tac_shell_timer.h" // Timepoint

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

