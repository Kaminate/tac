#pragma once

//#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/memory/tac_smart_ptr.h"
#include "tac-engine-core/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-engine-core/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-engine-core/shell/tac_shell_timer.h" // Timepoint
//#include "tac-engine-core/window/tac_sys_window_api.h"
//#include "tac-engine-core/window/tac_sim_window_api.h"
//#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
//#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
//#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h" // ImGuiSimFrameDraws
#include "tac-engine-core/settings/tac_settings_node.h" // SettingsNode

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
    };

    struct Config
    {
      String mName                       { "Tac" };
      String mStudioName                 { "Sleeping Studio" };

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer            {};
    };

    struct RenderParams
    {
      IState*        mOldState    {};
      IState*        mNewState    {};
      float          mT           {}; // [ 0, 1 ]
      Timestamp      mTimestamp   {}; // = Lerp( old timestamp, new timestamp, t )
    };

    using State = SmartPtr< IState >;

    App( const Config& );
    virtual ~App() = default;

    virtual void Init( Errors& ){};
    virtual void Update( Errors& ){};
    virtual void Render( RenderParams, Errors& ){};
    virtual void Present( Errors& ){};
    virtual void Uninit( Errors& ){};
    virtual auto GameState_Create() -> State;
    virtual void GameState_Update( IState* ) {}

    bool IsRenderEnabled() const;
    auto GetAppName() const -> StringView;
    auto GetStudioName() const -> StringView;

    static auto Create() -> App*;
    static auto Instance() -> App*;

    SettingsNode    mSettingsNode{};

  protected:
    Config          mConfig{};
  };
} // namespace Tac

