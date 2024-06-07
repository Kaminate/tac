#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_window_api.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-level-editor/tac_entity_selection.h"

namespace Tac
{

  struct LevelEditorWindowManager
  {
    void                Uninit( Errors& );
    void                Update( Errors& );


    bool                AllWindowsClosed();

    void                CreatePropertyWindow( Errors& );
    void                CreateGameWindow( Errors& );
    void                CreateMainWindow( Errors& );
    void                CreateSystemWindow( Errors& );
    void                CreateProfileWindow( Errors& );

    void                CreateInitialWindows( Errors& );
    WindowHandle        CreateDesktopWindow( StringView );

  private:

    bool                ShouldCreateWindowNamed( StringView );
    Json*               GetWindowsJson();
    WindowCreateParams  GetWindowsJsonData( StringView windowName );
    Json*               FindWindowJson( StringView windowName );
    void                UpdateCreatedWindowData();
  };
} // namespace Tac

