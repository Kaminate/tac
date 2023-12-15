#pragma once

#include "src/common/tac_core.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_vector.h"
#include "space/tac_space_types.h"
#include "space/tac_space.h"
#include "src/level_editor/tac_entity_selection.h"

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
    DesktopWindowHandle CreateDesktopWindow( StringView );

  private:

    bool                ShouldCreateWindowNamed( StringView );
    Json*               GetWindowsJson();
    struct DesktopAppCreateWindowParams GetWindowsJsonData( StringView windowName );
    Json*               FindWindowJson( StringView windowName );
    void                UpdateCreatedWindowData();
  };


} // namespace Tac

