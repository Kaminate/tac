#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  struct Debug3DDrawData;
  struct Creation;
  struct Shader;
  struct Soul;
  struct UI2DDrawData;

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
    void                          RenderEditorWidgetsSelection( Render::IContext*,
                                                                WindowHandle );
    void                          RenderEditorWidgets( Render::IContext*,
                                                       WindowHandle,
                                                       Errors& );
    void                          RenderEditorWidgetsLights( Render::IContext*,
                                                             WindowHandle,
                                                             Errors& );

    WindowHandle                  mWindowHandle             {};
    Soul*                         mSoul                     {};
    Render::ProgramHandle         mSpriteShader             {};
    Render::ProgramHandle         m3DShader                 {};
    Render::PipelineHandle        mSpritePipeline           {};
    Render::PipelineHandle        m3DPipeline               {};
    Debug3DDrawData*              mDebug3DDrawData          {};
    Debug3DDrawBuffers            mWorldBuffers             {};
    Debug3DDrawBuffers            mDebugBuffers             {};
    Mesh*                         mArrow                    {};
    Mesh*                         mCenteredUnitCube         {};
    v3                            mViewSpaceUnitMouseDir    {};
    v3                            mWorldSpaceMouseDir       {};
    float                         mArrowLen                 {};
    String                        mStatusMessage            {};
    Timestamp                     mStatusMessageEndTime     {};
    bool                          mCloseRequested           {};
  };

  const char* const gGameWindowName { "VirtualGamePlayer" };
}
