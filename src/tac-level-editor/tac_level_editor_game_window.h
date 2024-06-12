#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/string/tac_string.h"

#include "tac-level-editor/tac_level_editor_mouse_picking.h"

namespace Tac
{

  struct CreationGameWindow
  {
    static void Init( GizmoMgr*, CreationMousePicking*, SettingsNode, Errors& );
    static void Update( World*, Camera*, Errors& );
    static void Render( World*, Camera*, Errors& );
    static void SetStatusMessage( StringView, TimestampDifference );
    static bool sShowWindow;
  };

#if 0
  struct CreationGameWindow
  {
    // TODO: multiple game windows?
    CreationGameWindow();
    ~CreationGameWindow();
    static CreationGameWindow*    Instance;
    void                          Init( Errors& );
    void                          Update( World*, Camera*,Errors& );
    void                          Render( World*, Camera*, Errors& );

    void                          CameraUpdateControls( Camera* );
    void                          ImGuiOverlay( Camera*, Errors& );
    void                          PlayGame( Errors& );

    void                          SetStatusMessage( const StringView&,
                                                    const TimestampDifference& );

  private:
    void                          ImGuiCamera( Camera* );
    void                          RenderSelectionCircle(World*, Camera*);
    void                          RenderEditorWidgets( Render::IContext*,
                                                       WindowHandle,
                                                       Camera* ,
                                                       Errors& );

    void CameraWASDControls( Camera* );

  };
  const char* const gGameWindowName { "VirtualGamePlayer" };
#endif

}
