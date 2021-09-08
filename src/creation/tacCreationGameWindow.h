#pragma once

#include "src/common/string/tacString.h"
#include "src/common/math/tacVector3.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Entity;
  struct Model;
  struct Errors;
  struct Creation;
  struct UIRoot;
  struct Debug3DDrawData;
  struct UI2DDrawData;
  struct Soul;
  struct Shell;
  struct Shader;
  struct VertexFormat;
  struct CBuffer;
  struct DepthState;
  struct BlendState;
  struct RasterizerState;
  struct SamplerState;
  struct Mesh;
  struct DefaultCBufferPerObject;
  struct GamePresentation;
  struct SkyboxPresentation;

  struct CreationGameWindow
  {
    // TODO: multiple game windows?
    CreationGameWindow();
    ~CreationGameWindow();
    static CreationGameWindow*    Instance;
    void                          Init( Errors& );
    void                          Update( Errors& );
    void                          RenderGameWorldToGameWindow( Render::ViewHandle );
    void                          MousePickingInit();
    void                          MousePickingEntity( const Entity* entity, bool* hit, float* dist );
    void                          MousePickingAll();
    void                          ComputeArrowLen();
    void                          CameraControls();
    void                          CreateGraphicsObjects( Errors& );
    void                          DrawPlaybackOverlay( Errors& );
    void                          PlayGame( Errors& );
    DesktopWindowHandle           mDesktopWindowHandle;
    Soul*                         mSoul = nullptr;
    Render::ShaderHandle          m3DShader;
    Render::VertexFormatHandle    m3DVertexFormat;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    Render::VertexDeclarations    m3DvertexFormatDecls;
    Debug3DDrawData*              mDebug3DDrawData = nullptr;
    Mesh*                         mArrow = nullptr;
    Mesh*                         mCenteredUnitCube = nullptr;
    v3                            mWorldSpaceMouseDir = {};
    float                         mArrowLen = 0;
    String                        mStatusMessage;
    double                        mStatusMessageEndTime = 0;
  };


  const char* const gGameWindowName = "VirtualGamePlayer";
}
