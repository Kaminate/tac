#pragma once

//#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/system/tac_desktop_window.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-engine-core/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-engine-core/shell/tac_shell_timer.h" // Timepoint

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

