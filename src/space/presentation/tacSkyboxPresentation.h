#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Camera;

  struct SkyboxPresentation
  {
    ~SkyboxPresentation();
    void                          RenderSkybox( int viewWidth,
                                                int viewHeight,
                                                Render::ViewHandle viewId,
                                                StringView skyboxDir );
    void                          Init( Errors& );
    Camera*                       mCamera = nullptr;
    Render::VertexFormatHandle    mVertexFormat;
    Render::ShaderHandle          mShader;
    Render::ConstantBufferHandle  mPerFrame;
    Render::BlendStateHandle      mBlendState;
    Render::DepthStateHandle      mDepthState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    static const int              kVertexFormatDeclCount = 1;
    VertexDeclaration             mVertexDecls[ kVertexFormatDeclCount ];
    Errors                        mGetSkyboxTextureErrors;
    Errors                        mGetSkyboxMeshErrors;
  };
}
