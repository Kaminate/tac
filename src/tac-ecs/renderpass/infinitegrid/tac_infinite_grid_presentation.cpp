#include "tac_infinite_grid_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-std-lib/error/tac_assert.h"

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()

namespace Tac
{
  struct ConstantBuffer
  {
    enum CameraType : u32
    {
      CameraType_Perspective,
      CameraType_Orthographic,
    };

    static auto CPUToGPUCameraType( Camera::Type cpuCameraType ) -> CameraType
    {
      switch( cpuCameraType )
      {
        case Camera::Type::kOrthographic: return CameraType_Orthographic;
        case Camera::Type::kPerspective: return CameraType_Perspective;
        default: TAC_ASSERT_INVALID_CASE( cpuCameraType ); return {};
      }
    }

    m4         mInvView   {};
    m4         mInvProj   {};
    m4         mViewProj  {};
    v4         mCamPos_ws {};
    v4         mCamDir_ws {};
    CameraType mCamType   {};
  };

  static Render::ProgramHandle         sProgram;
  static Render::PipelineHandle        sPipeline;
  static Render::IShaderVar*           sShaderConstants;
  static Render::BufferHandle          sConstantBuffer;
  static Render::UpdateBufferParams    sConstantUpdateParams;
  static ConstantBuffer                sConstantData;
  static bool                          sInitialized;
  static bool                          sEnabled { true };

  // -----------------------------------------------------------------------------------------------

  void InfiniteGrid::Init( Errors& errors )
  {
    if( sInitialized )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    sProgram = renderDevice->CreateProgram(
      Render::ProgramParams{ .mInputs { "InfiniteGrid" }, },
      errors );
    TAC_CALL( sPipeline = renderDevice->CreatePipeline(
      Render::PipelineParams
      {
        .mProgram           { sProgram },
        .mBlendState   { Render::BlendState {
            .mSrcRGB   { Render::BlendConstants::One },
            .mDstRGB   { Render::BlendConstants::Zero },
            .mBlendRGB { Render::BlendMode::Add },
            .mSrcA     { Render::BlendConstants::Zero },
            .mDstA     { Render::BlendConstants::One },
            .mBlendA   { Render::BlendMode::Add },
          } },
        .mDepthState     { Render::DepthState {
            .mDepthTest  { true },
            .mDepthWrite { true },
            .mDepthFunc  { Render::DepthFunc::Less },
          }},
        .mRasterizerState { Render::RasterizerState
            {
              .mFillMode              { Render::FillMode::Solid },
              .mCullMode              { Render::CullMode::Back },
              .mFrontCounterClockwise { true },
              .mMultisample           {},
            }
        },
        .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
        .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
        .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
        .mName              { "infinite-grid-pso" },
      }, errors ) );
    TAC_CALL( sConstantBuffer = renderDevice->CreateBuffer(
      Render::CreateBufferParams
      {
        .mByteCount     { sizeof( ConstantBuffer ) },
        .mUsage         { Render::Usage::Dynamic },
        .mBinding       { Render::Binding::ConstantBuffer },
        .mOptionalName  { "infinite-grid-cbuf" },
      }, errors ) );
    sShaderConstants = renderDevice->GetShaderVariable( sPipeline, "sConstants" );
    sShaderConstants->SetResource( sConstantBuffer );
    sConstantUpdateParams = Render::UpdateBufferParams 
    {
      .mSrcBytes     { &sConstantData },
      .mSrcByteCount { sizeof( ConstantBuffer ) },
    };
    sInitialized = true;
  }

  void InfiniteGrid::Uninit()
  {
    if( sInitialized )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroyProgram( sProgram );
      renderDevice->DestroyPipeline( sPipeline );
      sInitialized = false;
    }
  }

  void InfiniteGrid::Render( Render::IContext* renderContext,
                             const Camera* camera,
                             const v2i viewSize,
                             const Render::TextureHandle dstColorTex,
                             const Render::TextureHandle dstDepthTex,
                             Errors& errors )
  {
    if( !sEnabled )
      return;

    sConstantData = ConstantBuffer
    {
      .mInvView = camera->ViewInv(),
      .mInvProj = camera->ProjInv( ( float )viewSize.x / ( float )viewSize.y ),
      .mViewProj = camera->Proj( ( float )viewSize.x / ( float )viewSize.y ) * camera->View(),
      .mCamPos_ws = v4( camera->mPos, 1 ),
      .mCamDir_ws = v4( camera->mForwards, 0 ),
      .mCamType = ConstantBuffer::CPUToGPUCameraType( camera->mType ),
    };
    TAC_RENDER_GROUP_BLOCK( renderContext, "Infinite Grid" );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetRenderTargets( Render::Targets{ .mColors{ dstColorTex }, .mDepth{ dstDepthTex }, } );
    TAC_CALL( renderContext->UpdateBuffer( sConstantBuffer, sConstantUpdateParams, errors ) );
    renderContext->SetPipeline( sPipeline );
    renderContext->CommitShaderVariables();
    renderContext->SetVertexBuffer( {} );
    renderContext->SetIndexBuffer( {} );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->Draw( Render::DrawArgs { .mVertexCount { 6 }, } );
  }

  void InfiniteGrid::DebugImGui()
  {
    ImGuiCheckbox( "Infinite Grid", &sEnabled );
  }

} // namespace Tac
#endif
