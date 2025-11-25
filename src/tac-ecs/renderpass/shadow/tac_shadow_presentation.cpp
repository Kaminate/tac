#include "tac_shadow_presentation.h" // self-inc

#if TAC_SHADOW_PRESENTATION_ENABLED()

#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_game_time.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"



namespace Tac
{
  // -----------------------------------------------------------------------------------------------


  struct ShadowPerFrame
  {
    m4 mView;
    m4 mProj;
  };

  struct ShadowPerObj
  {
    m4 mWorld;
  };

  struct ShadowModelVisitor : public ModelVisitor
  {
    void operator()( Model* model ) override;

    Render::IContext*             mRenderContext {};
    Render::TextureHandle         mShadowDepth;
    Render::VertexDeclarations    mVertexDeclarations;
    Errors                        mErrors;
  };

  // -----------------------------------------------------------------------------------------------

  struct ShadowLightVisitor : public LightVisitor
  {
    
    void operator()( Light* ) override;

    Render::IContext* mRenderContext {};
    Graphics*         graphics       {};
    Errors*           mErrors        {};
  };

  // -----------------------------------------------------------------------------------------------

  static Render::ProgramHandle  sShader;
  static Render::BufferHandle   sCBufPerFrame;
  static Render::BufferHandle   sCBufPerObj;
  static Render::PipelineHandle sPipeline;
  static bool                   sInitialized;

  // -----------------------------------------------------------------------------------------------

  static Render::TextureHandle CreateShadowMapDepth( const Light* light, Errors& errors )
  {
    const Render::Image image
    {
      .mWidth  { light->mShadowResolution },
      .mHeight { light->mShadowResolution },
      .mFormat { Render::TexFmt::kR32_float },
    };

    const Render::Binding binding
    {
      Render::Binding::DepthStencil |
      Render::Binding::ShaderResource
    };

    const Render::CreateTextureParams params
    {
      .mImage        { image },
      .mBinding      { binding },
      .mOptionalName { "shadowmap-depth" },
      .mStackFrame   { TAC_STACK_FRAME },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    return renderDevice->CreateTexture( params, errors );
  }


  static void CreateShadowMapFramebuffer( Light* light, Errors& errors )
  {
    light->mShadowMapDepth = CreateShadowMapDepth( light, errors );
  }

  static void CreateShadowMapResources( Light* light, Errors& errors )
  {
    if( !light->mCreatedRenderResources )
    {
      light->mCreatedRenderResources = true;
      CreateShadowMapFramebuffer( light, errors );
    }
  }

  static m4 GetLightView( const Light* light )
  {
    const Camera camera{ light->GetCamera() };
    return camera.View();
  }

  static m4 GetLightProj( const Light* light )
  {
    const Camera camera{ light->GetCamera() };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndc{ renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndc.mMinZ },
      .mNDCMaxZ       { ndc.mMaxZ },
      .mViewSpaceNear { camera.mNearPlane },
      .mViewSpaceFar  { camera.mFarPlane },
      .mAspectRatio   { 1 },
      .mFOVYRadians   { camera.mFovyrad }, // uhh should this be asserted to be 90 deg?
    };

    return m4::ProjPerspective( projParams );
  }

  static ShadowPerFrame GetPerFrame( const Light* light )
  {
    const m4 view{ GetLightView( light ) };
    const m4 proj{ GetLightProj( light ) };
    return ShadowPerFrame
    {
      .mView {view},
      .mProj {proj},
    };
  }

  //static ShadowPerObj   GetPerObj()
  //{
  //}

  static void UpdatePerFrameConstantBuffer( Render::IContext* renderContext,
                                            const Light* light,
                                            Errors & errors )
  {
    const ShadowPerFrame perFrame{ GetPerFrame( light ) }; 
    const Render::UpdateBufferParams updatePerFrame
    {
      .mSrcBytes     { &perFrame },
      .mSrcByteCount { sizeof(ShadowPerFrame) },
    };
    TAC_CALL( renderContext->UpdateBuffer( sCBufPerFrame, updatePerFrame, errors ) );
  }

  static void UpdatePerObjConstantBuffer( Render::IContext* renderContext,
                                          const Model* model,
                                          Errors& errors )
  {
    const ShadowPerObj perObj
    {
      .mWorld { model->mEntity->mWorldTransform },
    }; 

    const Render::UpdateBufferParams updatePerObj
    {
      .mSrcBytes     { &perObj },
      .mSrcByteCount { sizeof(ShadowPerObj) },
    };
    TAC_CALL( renderContext->UpdateBuffer( sCBufPerObj, updatePerObj, errors ) );
  }

  static Render::DepthState         ShadowGetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static Render::BlendState         ShadowGetBlendState()
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::Zero },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add },
    };
  }

  static Render::VertexDeclarations ShadowGetVtxDecls()
  {
    struct GameModelVtx
    {
      v3 mPos;
      v3 mNor;
    };

    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mNor ) },
    };

    Render::VertexDeclarations decls;
    decls.push_back( posDecl );
    decls.push_back( norDecl );
    return decls;
  }

  static Render::RasterizerState    ShadowGetRasterizer()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::Back },
      .mFrontCounterClockwise { true },
      .mMultisample           {},
    };
  }

  // -----------------------------------------------------------------------------------------------

  void ShadowModelVisitor::operator()( Model* model )
  {
    Render::IContext* renderContext{ mRenderContext };
    Errors& errors{ mErrors };
    const ModelAssetManager::Params meshParams
    {
      .mPath         { model->mModelPath },
      .mModelIndex   { model->mModelIndex },
      .mOptVtxDecls  { mVertexDeclarations },
    };

    Mesh* mesh{ ModelAssetManager::GetMesh( meshParams, errors ) };
    if( !mesh )
      return;

    TAC_CALL( UpdatePerObjConstantBuffer( renderContext, model, errors ) );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      TAC_RENDER_GROUP_BLOCK( renderContext,  subMesh.mName );
      const Render::DrawArgs drawArgs
      {
        .mIndexCount{ subMesh.mIndexCount },
      };

      renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
      renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
      renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      renderContext->Draw(drawArgs );
    }
  }

  void ShadowLightVisitor::operator()( Light* light )
  {
    Errors& errors{ *mErrors };
    if( !light->mCastsShadows )
      return;

    TAC_RENDER_GROUP_BLOCK( mRenderContext, String()
                            + "Light Shadow "
                            + ToString( ( UUID )light->mEntity->mEntityUUID ) );
    CreateShadowMapResources( light, errors );

    Render::IContext* renderContext{ mRenderContext };
    TAC_CALL( UpdatePerFrameConstantBuffer( renderContext, light, errors ) );
 
    const Render::Targets renderTargets
    {
      .mDepth{ light->mShadowMapDepth },
    };
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( v2i( light->mShadowResolution, light->mShadowResolution ) );
    renderContext->SetScissor( v2i( light->mShadowResolution, light->mShadowResolution ) );

    ShadowModelVisitor modelVisitor;
    modelVisitor.mShadowDepth = light->mShadowMapDepth;
    modelVisitor.mVertexDeclarations = ShadowGetVtxDecls();
    graphics->VisitModels( &modelVisitor );
  }

  // -----------------------------------------------------------------------------------------------

  void ShadowPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs     { "Shadow" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    sShader = renderDevice->CreateProgram( programParams, errors );

    const Render::CreateBufferParams createPerFrame
    {
      .mByteCount     { sizeof( ShadowPerFrame ) },
      .mUsage         { Render::Usage::Default },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "shadow-per-frame" },
    };

    const Render::CreateBufferParams createPerObj
    {
      .mByteCount     { sizeof( ShadowPerObj ) },
      .mUsage         { Render::Usage::Default },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "shadow-per-obj" },
    };

    TAC_CALL( sCBufPerFrame = renderDevice->CreateBuffer( createPerFrame, errors ) );
    TAC_CALL( sCBufPerObj = renderDevice->CreateBuffer( createPerObj, errors ) );

    const Render::PipelineParams pipelineParams
    {
      .mProgram           { sShader},
      .mBlendState        { ShadowGetBlendState() },
      .mDepthState        { ShadowGetDepthState() },
      .mRasterizerState   { ShadowGetRasterizer() },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { ShadowGetVtxDecls() },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "shadow-pipeline" },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    Render::IShaderVar* shaderPerFrame{ renderDevice->GetShaderVariable( sPipeline, "sPerFrame" ) };
    shaderPerFrame->SetResource( sCBufPerFrame );

    Render::IShaderVar* shaderPerObj{ renderDevice->GetShaderVariable( sPipeline, "sPerObj" ) };
    shaderPerObj->SetResource( sCBufPerObj );
    sInitialized = true;
  }

  void ShadowPresentation::Uninit()
  {
    if( sInitialized )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroyProgram( sShader );
      renderDevice->DestroyBuffer( sCBufPerFrame );
      renderDevice->DestroyBuffer( sCBufPerObj );
      renderDevice->DestroyPipeline( sPipeline );
      sInitialized = false;
    }
  }

  void ShadowPresentation::Render( World* world, Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::IContext::Scope renderContextScope{ renderDevice->CreateRenderContext( errors ) };
    Render::IContext* renderContext{ renderContextScope.GetContext() };

    Graphics* graphics { Graphics::From( world ) };

    ShadowLightVisitor lightVisitor;
    lightVisitor.graphics = graphics;
    lightVisitor.mErrors = &errors;
    lightVisitor.mRenderContext = renderContext;

    renderContext->DebugEventBegin( "Render Shadow Maps" );
    renderContext->SetPipeline( sPipeline );
    graphics->VisitLights( &lightVisitor );
    renderContext->DebugEventEnd();
    TAC_CALL( renderContext->Execute( errors ) );
  }

  void ShadowPresentation::DebugImGui()
  {
  }

} // namespace Tac
#endif
