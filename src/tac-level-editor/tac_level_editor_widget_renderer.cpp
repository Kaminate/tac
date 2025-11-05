#include "tac_level_editor_widget_renderer.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/tac_renderer_util.h" // PremultipliedAlpha
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-std-lib/math/tac_matrix4.h"

#if TAC_IS_WIDGET_RENDERER_ENABLED()

namespace Tac
{
  struct GameWindowVertex
  {
    v3 pos;
    v3 nor;
  };

  struct PerFrame
  {
    m4 mView;
    m4 mProj;
  };

  struct PerObj
  {
    m4 mWorld;
    v4 mColor;
  };


  static auto GetProj( WindowHandle viewHandle, const Camera* camera ) -> m4
  {
    
    const v2i windowSize{ AppWindowApi::GetSize( viewHandle ) };
    const float aspectRatio{ ( float )windowSize.x / ( float )windowSize.y };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    return m4::ProjPerspective( projParams );
  }

  static auto GetPerObjParams() -> Render::CreateBufferParams
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerObj ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "widget per obj" },
    };
  }

  static auto GetPerFrameParams() -> Render::CreateBufferParams
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerFrame ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "widget per frame" },
    };
  }

  static auto GetProgramParams3DTest() -> Render::ProgramParams
  {
    return Render::ProgramParams
    {
      .mInputs     { "3DTest" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  static auto GetRasterizerState() -> Render::RasterizerState
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           {},
    };
  }

  static auto GetBlendState() -> Render::BlendState
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::SrcA },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add},
    };
  }

  static auto GetDepthState() -> Render::DepthState
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static auto GetVtxDecls3D() -> Render::VertexDeclarations
  {
    const Render::VertexDeclaration posDecl
    {
        .mAttribute         { Render::Attribute::Position },
        .mFormat            { Render::VertexAttributeFormat::GetVector3() },
        .mAlignedByteOffset { TAC_OFFSET_OF( GameWindowVertex, pos ) },
    };

    const Render::VertexDeclaration norDecl
    {
        .mAttribute         { Render::Attribute::Normal },
        .mFormat            { Render::VertexAttributeFormat::GetVector3() },
        .mAlignedByteOffset { TAC_OFFSET_OF( GameWindowVertex, nor ) },
    };

    Render::VertexDeclarations m3DvertexFormatDecls;
    m3DvertexFormatDecls.push_back( posDecl );
    m3DvertexFormatDecls.push_back( norDecl );
    return m3DvertexFormatDecls;
  }

  // -----------------------------------------------------------------------------------------------

  WidgetRenderer WidgetRenderer::sInstance;

  auto WidgetRenderer::GetPipelineParams() ->   Render::PipelineParams
  {
    return Render::PipelineParams
    {
      .mProgram           { m3DShader },
      .mBlendState        { GetBlendState() },
      .mDepthState        { GetDepthState() },
      .mRasterizerState   { GetRasterizerState() },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { GetVtxDecls3D() },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "WidgetPipeline"},
    };
  };

  void WidgetRenderer::Init( Errors& errors )
  {
    Render::IDevice* renderDevice { Render::RenderApi::GetRenderDevice() };

    TAC_CALL( m3DShader = renderDevice->CreateProgram( GetProgramParams3DTest(), errors ) );
    TAC_CALL( m3DPipeline = renderDevice->CreatePipeline( GetPipelineParams(), errors ) );

    const Render::VertexDeclarations m3DvertexFormatDecls{ GetVtxDecls3D() };

    const ModelAssetManager::Params meshParms
    {
      .mPath        { "assets/editor/arrow.gltf" },
      .mOptVtxDecls { m3DvertexFormatDecls },
    };

    TAC_CALL( mArrow = ModelAssetManager::GetMesh( meshParms, errors ) );

    TAC_CALL( mBufferPerFrame = renderDevice->CreateBuffer( GetPerFrameParams(), errors ) );
    TAC_CALL( mBufferPerObj = renderDevice->CreateBuffer( GetPerObjParams(), errors ) );
    mShaderPerFrame = renderDevice->GetShaderVariable( m3DPipeline, "perFrame" );
    mShaderPerObj = renderDevice->GetShaderVariable( m3DPipeline, "perObj" );

    mShaderPerFrame->SetResource( mBufferPerFrame );
    mShaderPerObj->SetResource( mBufferPerObj );

  }

  void WidgetRenderer::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DShader);
    renderDevice->DestroyPipeline( m3DPipeline);
  }


  void WidgetRenderer::UpdatePerFrame( Render::IContext* renderContext,
                                       WindowHandle viewHandle,
                                       const Camera* camera,
                                       Errors& errors )
  {
    const m4 view { camera->View() };
    const m4 proj { GetProj( viewHandle, camera ) };
    const PerFrame perFrame
    {
      .mView { view },
      .mProj { proj },
    };

    const Render::UpdateBufferParams update
    {
      .mSrcBytes     { &perFrame },
      .mSrcByteCount { sizeof( PerFrame ) },
    };

    renderContext->UpdateBuffer( mBufferPerFrame, update, errors );
  }

  void WidgetRenderer::UpdatePerObject( Render::IContext* renderContext, int i, Errors& errors )
  {
    const v4 arrowColor{ GetAxisColor( i ) };
    const m4 world{ GetAxisWorld( i ) };

    const PerObj perObj
    {
      .mWorld { world },
      .mColor { arrowColor },
    };

    const Render::UpdateBufferParams update
    {
      .mSrcBytes     { &perObj },
      .mSrcByteCount { sizeof( PerObj ) },
    };

    renderContext->UpdateBuffer( mBufferPerObj, update, errors );
  }

  auto WidgetRenderer::GetAxisColor( int i ) ->   v4
  {
    auto gizmoMgr{ &GizmoMgr::sInstance };
    auto mousePicking{ &CreationMousePicking::sInstance };
    const v3 axises[ 3 ]{ v3( 1, 0, 0 ),
                          v3( 0, 1, 0 ),
                          v3( 0, 0, 1 ), };
    const v3 axis{ axises[ i ] };
    dynmc v4 color{ axis, 1 };
    const bool isWidgetHovered{ mousePicking->IsTranslationWidgetPicked( i ) };
    const bool isWidgetActive{ gizmoMgr->IsTranslationWidgetActive( i ) };
    if( isWidgetHovered || isWidgetActive )
    {
      dynmc float t { float( Sin( Timestep::GetElapsedTime() * 6.0 ) ) };
      t *= t;
      color = Lerp( v4( 1, 1, 1, 1 ), color, t );
    }

    return color;
  }

  auto WidgetRenderer::GetAxisWorld( const int i ) -> m4
  {
    auto gizmoMgr{ &GizmoMgr::sInstance };
    const m4 rots[]{ m4::RotRadZ( -3.14f / 2.0f ),
                     m4::Identity(),
                     m4::RotRadX( 3.14f / 2.0f ), };
    const v3 selectionGizmoOrigin{ gizmoMgr->mGizmoOrigin };
    const m4 world{ m4::Translate( selectionGizmoOrigin ) *
                    rots[ i ] *
                    m4::Scale( v3( 1, 1, 1 ) * gizmoMgr-> mArrowLen ) };
    return world;
  }

  void WidgetRenderer::RenderTranslationWidget( Render::IContext* renderContext,
                                                const WindowHandle viewHandle,
                                                const Camera* camera,
                                                Errors& errors )
  {
    if( auto gizmoMgr{ &GizmoMgr::sInstance };
        !gizmoMgr->mGizmosEnabled ||
        !gizmoMgr->mTranslationGizmoVisible )
      return;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Editor Selection" );
    renderContext->SetPipeline( m3DPipeline );
    TAC_CALL( UpdatePerFrame( renderContext, viewHandle, camera, errors ) );

    for( int i{}; i < 3; ++i )
    {
      TAC_CALL( UpdatePerObject( renderContext, i, errors ) );
      renderContext->CommitShaderVariables();
      for( const SubMesh& subMesh : mArrow->mSubMeshes )
      {
        const Render::DrawArgs drawArgs{ .mIndexCount { subMesh.mIndexCount }, };
        renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
        renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
        renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
        renderContext->Draw( drawArgs );
      }
    }
  }
} // namespace Tac

#endif
