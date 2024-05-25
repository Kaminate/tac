#pragma once

#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-engine-core/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-engine-core/shell/tac_shell_timer.h" // Timepoint
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h" // ImGuiSimFrameDraws

namespace Tac { struct SimWindowApi; struct SimKeyboardApi; }
namespace Tac { struct SysWindowApi; struct SysKeyboardApi; }
namespace Tac
{
  struct App
  {
    struct IState
    {
      virtual ~IState() = default;

      FrameIndex         mFrameIndex    {};
      Timestamp          mTimestamp     {};
      Timepoint          mTimepoint     {};
      ImGuiSimFrameDraws mImGuiDraws    {};
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
      const SysWindowApi*   mWindowApi   {};
      const SysKeyboardApi* mKeyboardApi {};
    };

    struct UpdateParams
    {
      const SimWindowApi*   mWindowApi   {};
      const SimKeyboardApi* mKeyboardApi {};
    };

    struct RenderParams
    {
      const SysWindowApi*   mWindowApi   {};
      const SysKeyboardApi* mKeyboardApi {};
      IState*               mOldState    {};
      IState*               mNewState    {};
      float                 mT           {}; // [ 0, 1 ]
      Timestamp             mTimestamp   {}; // = Lerp( old timestamp, new timestamp, t )
    };

    struct PresentParams
    {
      const SysWindowApi*   mWindowApi   {};
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

