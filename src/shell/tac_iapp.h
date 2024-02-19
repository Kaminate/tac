#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_core.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/containers/tac_list.h"

namespace Tac
{
  struct App
  {
    struct IState
    {
      virtual ~IState() = default;
    };

    struct Config
    {
      String mName;
      String mStudioName = "Sleeping Studio";

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer = false;
    };

    App(const Config& config ) : mConfig( config ) {}
    virtual ~App() {};

    virtual void Init( Errors& ) {};
    virtual void Update( Errors& ) {};
    virtual void Uninit( Errors& ) {};
    virtual void Render( IState*, IState*, float, Errors& ) {};
    virtual IState* GetGameState() { return nullptr; }

    static App*  Create();
    bool         IsRenderEnabled() const { return !mConfig.mDisableRenderer; }

    Config mConfig;
  };
} // namespace Tac

