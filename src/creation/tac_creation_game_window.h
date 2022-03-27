#pragma once

#include "src/common/string/tac_string.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/graphics/tac_renderer.h"

namespace Tac
{
  struct BlendState;
  struct CBuffer;
  struct Creation;
  struct Debug3DDrawData;
  struct DefaultCBufferPerObject;
  struct DepthState;
  struct Entity;
  struct Errors;
  struct GamePresentation;
  struct Light;
  struct Mesh;
  struct Model;
  struct RasterizerState;
  struct SamplerState;
  struct Shader;
  struct Shell;
  struct SkyboxPresentation;
  struct Soul;
  struct UI2DDrawData;
  struct UIRoot;
  struct VertexFormat;

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
    DesktopWindowHandle           mDesktopWindowHandle;
    Soul*                         mSoul = nullptr;
    Render::ShaderHandle          m3DShader;
    Render::VertexFormatHandle    m3DVertexFormat;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::BlendStateHandle      mAlphaBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    Render::VertexDeclarations    m3DvertexFormatDecls;
    Debug3DDrawData*              mDebug3DDrawData = nullptr;
    Mesh*                         mArrow = nullptr;
    Mesh*                         mCenteredUnitCube = nullptr;
    v3                            mViewSpaceUnitMouseDir = {};
    v3                            mWorldSpaceMouseDir = {};
    float                         mArrowLen = 0;
    String                        mStatusMessage;
    double                        mStatusMessageEndTime = 0;
    bool                          mCloseRequested = false;
  };


  const char* const gGameWindowName = "VirtualGamePlayer";
}
