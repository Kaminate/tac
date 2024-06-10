#include "tac_level_editor_widget_renderer.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
//#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/graphics/tac_renderer_util.h" // PremultipliedAlpha
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/math/tac_matrix4.h"


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

  static m4 GetProj( WindowHandle viewHandle, const Camera* camera )
  {
    SysWindowApi windowApi;
    const v2i windowSize{ windowApi.GetSize( viewHandle ) };
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

  static Render::CreateBufferParams GetPerObjParams()
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerObj ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "widget per obj" },
    };
  }

  static Render::CreateBufferParams GetPerFrameParams()
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerFrame ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "widget per frame" },
    };
  }

  static Render::ProgramParams GetProgramParams3DTest()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DTest" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  static Render::RasterizerState GetRasterizerState()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           { false },
    };
  }

  static Render::BlendState GetBlendState()
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

  static Render::DepthState GetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static Render::VertexDeclarations GetVtxDecls3D()
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

  Render::PipelineParams WidgetRenderer::GetPipelineParams()
  {
    return Render::PipelineParams
    {
      .mProgram           { m3DShader },
      .mBlendState        { GetBlendState() },
      .mDepthState        { GetDepthState() },
      .mRasterizerState   { GetRasterizerState() },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
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

    TAC_CALL( mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                               0,
                                                               m3DvertexFormatDecls,
                                                               errors ) );

    TAC_CALL( mBufferPerFrame = renderDevice->CreateBuffer( GetPerFrameParams(), errors ) );
    TAC_CALL( mBufferPerObj = renderDevice->CreateBuffer( GetPerObjParams(), errors ) );
    mShaderPerFrame = renderDevice->GetShaderVariable( m3DPipeline, "perFrame" );
    mShaderPerObj = renderDevice->GetShaderVariable( m3DPipeline, "perObj" );

    mShaderPerFrame->SetBuffer( mBufferPerFrame );
    mShaderPerObj->SetBuffer( mBufferPerObj );

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
      .mView{ view },
      .mProj{ proj },
    };

    const Render::UpdateBufferParams update
    {
      .mSrcBytes{ &perFrame },
      .mSrcByteCount{ sizeof( PerFrame ) },
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
      .mSrcBytes{ &perObj },
      .mSrcByteCount{ sizeof( PerObj ) },
    };

    renderContext->UpdateBuffer( mBufferPerObj, update, errors );
  }

  v4 WidgetRenderer::GetAxisColor( int i )
  {
    const v3 axises[ 3 ]
    {
      v3( 1, 0, 0 ),
      v3( 0, 1, 0 ),
      v3( 0, 0, 1 ),
    };

    const v3 axis{ axises[ i ] };

    const Render::PremultipliedAlpha axisPremultipliedColor{
        Render::PremultipliedAlpha::From_sRGB( axis ) };

    // is the widget hovered
    const bool picked{
      pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
      pickData.arrowAxis == i };

    // is the widget active
    const bool usingTranslationArrow{
      gCreation.mSelectedGizmo &&
      gCreation.mTranslationGizmoDir == axis };

    const bool shine { picked || usingTranslationArrow };

    Render::PremultipliedAlpha arrowColor { axisPremultipliedColor };
    if( shine )
    {
      float t { float( Sin( Timestep::GetElapsedTime() * 6.0 ) ) };
      t *= t;
      arrowColor.mColor = Lerp( v4( 1, 1, 1, 1 ), axisPremultipliedColor.mColor, t );
    }

    return arrowColor.mColor;
  }

  m4 WidgetRenderer::GetAxisWorld( int i )
  {
    const m4 rots[]{
      m4::RotRadZ( -3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( 3.14f / 2.0f ), };

    const v3 selectionGizmoOrigin{ gCreation.mSelectedEntities.GetGizmoOrigin() };

    const m4 world
    {
      m4::Translate( selectionGizmoOrigin ) *
      rots[ i ] *
      m4::Scale( v3( 1, 1, 1 ) * mArrowLen )
    };

    return world;
  }

  void WidgetRenderer::RenderTranslationWidget( Render::IContext* renderContext,
                                                const WindowHandle viewHandle,
                                                const Camera* camera,
                                                Errors& errors )
  {
    if( !sGizmosEnabled || gCreation.mSelectedEntities.empty() )
      return;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Editor Selection" );

    renderContext->SetPipeline( m3DPipeline );
    TAC_CALL( UpdatePerFrame( renderContext, viewHandle, camera, errors ) );

    for( int i{}; i < 3; ++i )
    {
      TAC_CALL( UpdatePerObject( renderContext, i, errors ) );

      const Mesh* mesh{ mArrow };
      for( const SubMesh& subMesh : mesh->mSubMeshes )
      {
        const Render::DrawArgs drawArgs
        {
          .mIndexCount { subMesh.mIndexCount },
        };

        renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
        renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
        renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
        renderContext->Draw( drawArgs );
      }
    }
  }
} // namespace Tac
