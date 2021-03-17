#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacErrorHandling.h"

namespace Tac
{
  struct Camera;

  struct SkyboxPresentation
  {
    ~SkyboxPresentation();
    void                          RenderSkybox( const Camera* camera,
                                                int viewWidth,
                                                int viewHeight,
                                                Render::ViewHandle viewId,
                                                StringView skyboxDir );
    void                          Init( Errors& );
    Render::VertexFormatHandle    mVertexFormat;
    Render::ShaderHandle          mShader;
    Render::ConstantBufferHandle  mPerFrame;
    Render::BlendStateHandle      mBlendState;
    Render::DepthStateHandle      mDepthState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    VertexDeclarations            mVertexDecls;
    Errors                        mGetSkyboxTextureErrors;
    Errors                        mGetSkyboxMeshErrors;
  };
}
