#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{

  struct Camera;
  struct DesktopWindow;


  struct SkyboxPresentation
  {
    ~SkyboxPresentation();
    void RenderSkybox( int viewWidth,
                       int viewHeight,
                       Render::ViewId viewId,
                       const String& skyboxDir );
    void Init( Errors& errors );

    Camera* mCamera = nullptr;
    DesktopWindow* mDesktopWindow = nullptr;

    Render::VertexFormatHandle mVertexFormat;
    Render::ShaderHandle mShader;
    Render::ConstantBufferHandle mPerFrame;
    Render::BlendStateHandle mBlendState;
    Render::DepthStateHandle mDepthState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle mSamplerState;

    static const int kVertexFormatDeclCount = 1;
    VertexDeclaration mVertexDecls[ kVertexFormatDeclCount ];
  };
}
