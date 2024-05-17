#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/tac_space.h"

namespace Tac::Render
{
  struct BlendState;
  struct DefaultCBufferPerObject;
  struct DepthState;
  struct RasterizerState;
  struct SamplerState;
  //struct VertexFormat;
}

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
    void                          UpdateGizmo();

    //                            Render
    void                          RenderEditorWidgets( Render::ViewHandle );
    void                          RenderEditorWidgetsSelection( Render::ViewHandle );
    void                          RenderEditorWidgetsLights( Render::ViewHandle );
    void                          RenderEditorWidgetsPicking( Render::ViewHandle );

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
    WindowHandle           mWindowHandle;
    Soul*                         mSoul {};
    Render::ShaderHandle          m3DShader;
    Render::VertexFormatHandle    m3DVertexFormat;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::BlendStateHandle      mAlphaBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    Render::VertexDeclarations    m3DvertexFormatDecls;
    Debug3DDrawData*              mDebug3DDrawData {};
    Mesh*                         mArrow {};
    Mesh*                         mCenteredUnitCube {};
    v3                            mViewSpaceUnitMouseDir {};
    v3                            mWorldSpaceMouseDir {};
    float                         mArrowLen {};
    String                        mStatusMessage;
    Timestamp                     mStatusMessageEndTime;
    bool                          mCloseRequested {};
  };


  const char* const gGameWindowName = "VirtualGamePlayer";
}
