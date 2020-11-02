#pragma once

#include "src/common/tacString.h"
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
  struct RenderView;
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
    void                          RenderGameWorldToGameWindow();
    void                          MousePickingInit();
    void                          MousePickingEntity( const Entity* entity, bool* hit, float* dist );
    void                          MousePickingAll();
    void                          AddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& cbuf );
    void                          ComputeArrowLen();
    void                          CameraControls();
    void                          CreateGraphicsObjects( Errors& );
    void                          DrawPlaybackOverlay( Errors& );
    void                          PlayGame( Errors& );
    DesktopWindowHandle           mDesktopWindowHandle;
    Soul*                         mSoul = nullptr;
    Render::ShaderHandle          m3DShader;
    Render::VertexFormatHandle    m3DVertexFormat;
    Render::ConstantBufferHandle  mPerFrame;
    Render::ConstantBufferHandle  mPerObj;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    static const int              k3DvertexFormatDeclCount = 1;
    VertexDeclaration             m3DvertexFormatDecls[k3DvertexFormatDeclCount ];
    Debug3DDrawData*              mDebug3DDrawData = nullptr;
    GamePresentation*             mGamePresentation = nullptr;
    SkyboxPresentation*           mSkyboxPresentation = nullptr;
    Mesh*                         mArrow = nullptr;
    Mesh*                         mCenteredUnitCube = nullptr;
    v3                            mWorldSpaceMouseDir = {};
    float                         mArrowLen = 0;
    String                        mStatusMessage;
    double                        mStatusMessageEndTime = 0;
  };


  const char* const gGameWindowName = "VirtualGamePlayer";
}
