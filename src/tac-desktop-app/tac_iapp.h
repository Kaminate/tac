#pragma once

//#include "tac-engine-core/hid/tac_keyboard_api.h"
//#include "tac-engine-core/window/tac_window_api.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/shell/tac_shell_timestep.h" // FrameIndex
#include "tac-engine-core/shell/tac_shell_timestamp.h" // Timestamp
#include "tac-engine-core/shell/tac_shell_timer.h" // Timepoint
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h" // ImGuiSimFrameDraws

 // ImGuiSimFrameDraws::mWindowDraws:mDrawData::~UI2DDrawData
//#include "tac-engine-core/graphics/ui/tac_ui_2d.h"

namespace Tac { struct SimWindowApi; struct SimKeyboardApi; }
namespace Tac { struct SysWindowApi; struct SysKeyboardApi; }
namespace Tac
{
  struct App
  {
    struct IState
    {
      virtual ~IState() = default;

      FrameIndex         mFrameIndex{};
      Timestamp          mTimestamp{};
      Timepoint          mTimepoint{};
      ImGuiSimFrameDraws mImGuiDraws;
    };

    struct Config
    {
      String mName;
      String mStudioName = "Sleeping Studio";

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer = false;
    };

    struct SimInitParams
    {
      SimWindowApi*   mWindowApi{};
      SimKeyboardApi* mKeyboardApi{};
    };

    struct SimUpdateParams
    {
      SimWindowApi*   mWindowApi{};
      SimKeyboardApi* mKeyboardApi{};
    };

    struct SysRenderParams
    {
      SysWindowApi*   mWindowApi{};
      SysKeyboardApi* mKeyboardApi{};
      IState*         mOldState;
      IState*         mNewState;
      float           mT; // [0-1]
    };

    App(const Config& config ) : mConfig( config ) {}
    virtual ~App() {};

    virtual void Init( SimInitParams, Errors& ) {};
    virtual void Update( SimUpdateParams, Errors& ) {};
    virtual void Uninit( Errors& ) {};


    virtual void Render( SysRenderParams, Errors& ) {};
    virtual IState* GetGameState() { return nullptr; }

    static App*  Create();
    bool         IsRenderEnabled() const { return !mConfig.mDisableRenderer; }

    Config          mConfig;
  };
} // namespace Tac

