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
#include "tac-level-editor/tac_level_editor_icon_renderer.h"
#include "tac-level-editor/tac_level_editor_widget_renderer.h"

namespace Tac
{
  struct Creation;
  struct Shader;
  struct Soul;

  struct CreationGameWindow
  {
    // TODO: multiple game windows?
    CreationGameWindow();
    ~CreationGameWindow();
    static CreationGameWindow*    Instance;
    void                          Init( Errors& );
    void                          Update( Errors& );
    void                          Render( Errors& );
    void                          UpdateGizmo();

    //                            Render
    void                          RenderEditorWidgetsPicking();

    //                            MousePicking
    void                          MousePickingInit();
    void                          MousePickingAll();
    void                          MousePickingEntityLight( const Light*, bool* hit, float* dist );
    void                          MousePickingEntityModel( const Model*, bool* hit, float* dist );
    void                          MousePickingEntity( const Entity*, bool* hit, float* dist );
    void                          MousePickingEntities();
    void                          MousePickingGizmos();
    void                          MousePickingSelection();

    void                          ComputeArrowLen();
    void                          CameraUpdateControls();
    void                          CreateGraphicsObjects( Errors& );
    void                          ImGuiOverlay( Errors& );
    void                          PlayGame( Errors& );

    void                          SetStatusMessage( const StringView&, const TimestampDifference& );

  private:
    void                          RenderSelectionCircle();
    void                          RenderEditorWidgets( Render::IContext*,
                                                       WindowHandle,
                                                       Errors& );

    WindowHandle                  mWindowHandle             {};
    Soul*                         mSoul                     {};
    Debug3DDrawBuffers            mWorldBuffers             {};
    Mesh*                         mArrow                    {};
    Mesh*                         mCenteredUnitCube         {};
    v3                            mViewSpaceUnitMouseDir    {};
    v3                            mWorldSpaceMouseDir       {};
    float                         mArrowLen                 {};
    String                        mStatusMessage            {};
    Timestamp                     mStatusMessageEndTime     {};
    bool                          mCloseRequested           {};
    SettingsNode                  mSettingsNode             {};
    IconRenderer                  mIconRenderer             {};
    WidgetRenderer                mWidgetRenderer           {};
  };

  const char* const gGameWindowName { "VirtualGamePlayer" };
}
