#include "tac_shadow_presentation.h" // self-inc

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
//#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

//#include <cmath> // std::fmod

namespace Tac
{


  static Render::ProgramHandle sShader;

  struct ShadowModelVisitor : public ModelVisitor
  {
    void operator()( Model* model ) override;

    Render::ViewHandle            mViewHandle;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::SamplerStateHandle    mSamplerState;
    Render::TextureHandle         mShadowDepth;
    Render::VertexFormatHandle    mVertexFormatHandle;
    Render::VertexDeclarations    mVertexDeclarations;
    Render::RasterizerStateHandle mRasterizerStateHandle;
  };

  void ShadowModelVisitor::operator()( Model* model )
  {
    Errors errors;
    Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                         model->mModelIndex,
                                                         mVertexDeclarations,
                                                         errors );
    if( !mesh )
      return;

    const Render::DefaultCBufferPerObject objBuf
    {
      .World = model->mEntity->mWorldTransform,
      .Color = Render::PremultipliedAlpha::From_sRGB(  model->mColorRGB ),
    };

    const Render::BufferHandle hPerObj = Render::DefaultCBufferPerObject::Handle;
    const int perObjSize = sizeof( Render::DefaultCBufferPerObject );

    Render::UpdateConstantBuffer( hPerObj, &objBuf, perObjSize, TAC_STACK_FRAME );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      Render::BeginGroup( subMesh.mName, TAC_STACK_FRAME );
      Render::SetShader( sShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerStateHandle );
      Render::SetSamplerState( { mSamplerState } );
      Render::SetDepthState( mDepthState );
      Render::SetVertexFormat( mVertexFormatHandle );
      Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      Render::Submit( mViewHandle, TAC_STACK_FRAME );
      Render::EndGroup( TAC_STACK_FRAME );
    }
  }

  static Render::TextureHandle CreateShadowMapDepth( const Light* light )
  {
    const Render::Format format
    {
      .mElementCount = 1,
      .mPerElementByteCount = 4,
      .mPerElementDataType = Render::GraphicsType::real,
    };

    const Render::Image image
    {
      .mWidth{ light->mShadowResolution },
      .mHeight{ light->mShadowResolution },
      .mFormat{ format },
    };

    const Render::CreateTextureParams params
    {
      .mImage{ image },
      .mBinding = Render::Binding::DepthStencil | Render::Binding::ShaderResource,
      .mOptionalName = "shadowmap-depth",
    };

    Render::TextureHandle textureHandleDepth = Render::CreateTexture( params, TAC_STACK_FRAME );
    return textureHandleDepth;

  }


  static void CreateShadowMapFramebuffer( Light* light )
  {
    Render::TextureHandle textureHandleDepth = CreateShadowMapDepth( light );
    Render::FramebufferTextures framebufferTextures = { textureHandleDepth };
    Render::FramebufferHandle framebufferHandle
      = Render::CreateFramebufferForRenderToTexture( framebufferTextures, TAC_STACK_FRAME );

    light->mShadowFramebuffer = framebufferHandle;
    light->mShadowMapDepth = textureHandleDepth;
  }

  static void CreateShadowMapResources( Light* light )
  {
    if( light->mCreatedRenderResources )
      return;
    light->mCreatedRenderResources = true;
    light->mShadowView = Render::CreateView();
    CreateShadowMapFramebuffer( light );
  }

  struct ShadowLightVisitor : public LightVisitor
  {
    Render::DefaultCBufferPerFrame GetPerFrameData( const Light* light );
    void operator()( Light* light ) override;

    Graphics* graphics{};
  };


  Render::DefaultCBufferPerFrame ShadowLightVisitor::GetPerFrameData( const Light* light )
  {
    const Camera camera = light->GetCamera();
    const Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    const Render::NDCAttribs ndc = renderDevice->GetInfo().mNDCAttribs;
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ = ndc.mMinZ,
      .mNDCMaxZ = ndc.mMaxZ,
      .mViewSpaceNear = camera.mNearPlane,
      .mViewSpaceFar = camera.mFarPlane,
      .mAspectRatio = 1,
      .mFOVYRadians = camera.mFovyrad, // uhh should this be asserted to be 90 deg?
    };

    const m4 proj = m4::ProjPerspective(projParams);
    const float w = ( float )light->mShadowResolution;
    const float h = ( float )light->mShadowResolution;
    const Timestamp elapsedSeconds = Timestep::GetElapsedTime();

    return Render::DefaultCBufferPerFrame
    {
      .mView = camera.View(),
      .mProjection = proj,
      .mFar = camera.mFarPlane,
      .mNear = camera.mNearPlane,
      .mGbufferSize = { w, h },
      .mSecModTau = ( float )Fmod( elapsedSeconds, 6.2831853 ),
    };
  }

  void ShadowLightVisitor::operator()( Light* light )
  {
    if( !light->mCastsShadows )
      return;

    TAC_RENDER_GROUP_BLOCK( String()
                            + "Light Shadow "
                            + ToString( ( UUID )light->mEntity->mEntityUUID ) );
    CreateShadowMapResources( light );

    const Render::DefaultCBufferPerFrame perFrameData = GetPerFrameData( light );
    const Render::BufferHandle hPerFrame = Render::DefaultCBufferPerFrame::Handle;
    const int perFrameSize = sizeof( Render::DefaultCBufferPerFrame );


    Render::SetViewFramebuffer( light->mShadowView, light->mShadowFramebuffer );
    Render::SetViewport( light->mShadowView, Render::Viewport( light->mShadowResolution,
                                                               light->mShadowResolution ) );
    Render::SetViewScissorRect( light->mShadowView, Render::ScissorRect( light->mShadowResolution,
                                                                         light->mShadowResolution ) );
    Render::UpdateConstantBuffer( hPerFrame,
                                  &perFrameData,
                                  perFrameSize,
                                  TAC_STACK_FRAME );


    ShadowModelVisitor modelVisitor;
    modelVisitor.mViewHandle = light->mShadowView;
    modelVisitor.mDepthState = GamePresentationGetDepthState();
    modelVisitor.mBlendState = GamePresentationGetBlendState();
    modelVisitor.mSamplerState = GamePresentationGetSamplerState();
    modelVisitor.mVertexDeclarations = GamePresentationGetVertexDeclarations();
    modelVisitor.mVertexFormatHandle = GamePresentationGetVertexFormat();
    modelVisitor.mRasterizerStateHandle = GamePresentationGetRasterizerState();
    modelVisitor.mShadowDepth = light->mShadowMapDepth;

    graphics->VisitModels( &modelVisitor );
  }

  void        ShadowPresentationInit( Errors& errors )
  {
    sShader = Render::CreateShader(  "Shadow" , TAC_STACK_FRAME );
  }

  void        ShadowPresentationUninit()
  {
  }

  void        ShadowPresentationRender( World* world )
  {
    TAC_RENDER_GROUP_BLOCK( "Render Shadow Maps" );
    Graphics* graphics = GetGraphics( world );

    ShadowLightVisitor lightVisitor;
    lightVisitor.graphics = graphics;

    graphics->VisitLights( &lightVisitor );
  }

  void        ShadowPresentationDebugImGui( Graphics* graphics )
  {
    if( !ImGuiCollapsingHeader( "Shadow Presentation" ) )
      return;
  }
}
