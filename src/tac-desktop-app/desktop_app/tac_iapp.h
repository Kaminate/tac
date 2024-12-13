#pragma once

#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-engine-core/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-engine-core/shell/tac_shell_timer.h" // Timepoint
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h" // ImGuiSimFrameDraws

#define TAC_SINGLE_THREADED() 1

namespace Tac
{
  struct App
  {
    struct IState
    {
      virtual ~IState() = default;

      FrameIndex         mFrameIndex          {};
      Timestamp          mTimestamp           {};
      Timepoint          mTimepoint           {};
      ImGuiSimFrame      mImGuiSimFrame       {};
    };

    struct Config
    {
      String mName                       { "Tac" };
      String mStudioName                 { "Sleeping Studio" };

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer            {};
    };

    struct InitParams
    {
      SysWindowApi   mWindowApi   {};
      SysKeyboardApi mKeyboardApi {};
    };

    struct UpdateParams
    {
      SimWindowApi   mWindowApi   {};
      SimKeyboardApi mKeyboardApi {};
    };

    struct RenderParams
    {
      SysWindowApi   mWindowApi   {};
      SysKeyboardApi mKeyboardApi {};
      IState*        mOldState    {};
      IState*        mNewState    {};
      float          mT           {}; // [ 0, 1 ]
      Timestamp      mTimestamp   {}; // = Lerp( old timestamp, new timestamp, t )
    };

    struct PresentParams
    {
      SysWindowApi   mWindowApi   {};
    };

    App( const Config& config ) : mConfig( config ) {}
    virtual ~App() {};

    virtual void    Init( InitParams, Errors& )       {};
    virtual void    Update( UpdateParams, Errors& )   {};
    virtual void    Render( RenderParams, Errors& )   {};
    virtual void    Present( PresentParams, Errors& ) {};
    virtual void    Uninit( Errors& )                 {};

    virtual IState* GetGameState();
    bool            IsRenderEnabled() const;
    StringView      GetAppName() const;
    StringView      GetStudioName() const;

    static App*     Create();

    SettingsNode    mSettingsNode{};

  protected:
    Config          mConfig;
  };
} // namespace Tac

