#include "src/space/presentation/tac_skybox_presentation.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/string/tac_string.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/assetmanagers/tac_model_asset_manager.h"
#include "src/common/assetmanagers/tac_mesh.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_util.h"
#include "src/common/profile/tac_profile.h"

namespace Tac
{

  static Render::VertexFormatHandle    mVertexFormat;
  static Render::ShaderHandle          mShader;
  static Render::BlendStateHandle      mBlendState;
  static Render::DepthStateHandle      mDepthState;
  static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerStateHandle    mSamplerState;
  static Render::VertexDeclarations    mVertexDecls;
  static Errors                        mGetSkyboxTextureErrors;
  static Errors                        mGetSkyboxMeshErrors;

  void SkyboxPresentationUninit()
  {
    Render::DestroyShader( mShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( mVertexFormat, TAC_STACK_FRAME );
  }

  void SkyboxPresentationInit( Errors& errors )
  {

    mShader = Render::CreateShader( Render::ShaderSource::FromPath( "Skybox" ), TAC_STACK_FRAME );

    mVertexDecls = { Render::VertexDeclaration {
      .mAttribute = Render::Attribute::Position,
      .mTextureFormat{ .mElementCount = 3,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real},
      .mAlignedByteOffset = 0, } };

    mVertexFormat = Render::CreateVertexFormat( mVertexDecls,
                                                mShader,
                                                TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mVertexFormat, "skybox-fmt" );
    TAC_HANDLE_ERROR( errors );


    mBlendState = Render::CreateBlendState( { .mSrcRGB = Render::BlendConstants::One,
                                              .mDstRGB = Render::BlendConstants::Zero,
                                              .mBlendRGB = Render::BlendMode::Add,
                                              .mSrcA = Render::BlendConstants::Zero,
                                              .mDstA = Render::BlendConstants::One,
                                              .mBlendA = Render::BlendMode::Add}, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "skybox-blend" );

    mDepthState = Render::CreateDepthState( { .mDepthTest = true,
                                              .mDepthWrite = true,
                                              .mDepthFunc = Render::DepthFunc::LessOrEqual},
                                            TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "skybox-depth" );

    mRasterizerState = Render::CreateRasterizerState( { .mFillMode = Render::FillMode::Solid,
                                                        .mCullMode = Render::CullMode::None, // todo
                                                        .mFrontCounterClockwise = true,
                                                        .mScissor = true,
                                                        .mMultisample = false},
                                                      TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState, "skybox-rast" );
    TAC_HANDLE_ERROR( errors );

    mSamplerState = Render::CreateSamplerState( { .mFilter = Render::Filter::Linear }, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState, "skybox-samp" );
    TAC_HANDLE_ERROR( errors );
  }

  void SkyboxPresentationRender( const Camera* camera,
                                 const int viewWidth,
                                 const int viewHeight,
                                 const Render::ViewHandle viewId,
                                 const AssetPathStringView& skyboxDir )
  {
    /*TAC_PROFILE_BLOCK*/;
    const AssetPathStringView defaultSkybox = "assets/skybox/daylight";
    const AssetPathStringView skyboxDirToUse = skyboxDir.empty() ? defaultSkybox : skyboxDir;
    const Render::TextureHandle cubemap = TextureAssetManager::GetTextureCube( skyboxDirToUse,
                                                                               mGetSkyboxTextureErrors );
    TAC_ASSERT( mGetSkyboxTextureErrors.empty() );
    if( !cubemap.IsValid() )
      return;
    Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/Box.gltf",
                                                         0,
                                                         mVertexDecls,
                                                         mGetSkyboxMeshErrors );
    TAC_ASSERT( mGetSkyboxMeshErrors.empty() );
    if( !mesh )
      return;
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane, camera->mNearPlane, a, b );
    const float aspect = ( float )viewWidth / ( float )viewHeight;
    const m4 view = m4::ViewInv( v3( 0, 0, 0 ),
                                 camera->mForwards,
                                 camera->mRight,
                                 camera->mUp );
    const DefaultCBufferPerFrame perFrame{ .mView = view,
                                           .mProjection = camera->Proj( a, b, aspect ),
                                           .mFar = camera->mFarPlane,
                                           .mNear = camera->mNearPlane,
                                           .mGbufferSize = { ( float )viewWidth, ( float )viewHeight } };
    const SubMesh* subMesh = &mesh->mSubMeshes[ 0 ];
    Render::SetViewport( viewId, Render::Viewport( viewWidth, viewHeight ) );
    Render::SetViewScissorRect( viewId, Render::ScissorRect( viewWidth, viewHeight ) );
    Render::UpdateConstantBuffer( DefaultCBufferPerFrame::Handle,
                                  &perFrame,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );
    Render::SetVertexBuffer( subMesh->mVertexBuffer, 0, 0 );
    Render::SetIndexBuffer( subMesh->mIndexBuffer, 0, subMesh->mIndexCount );
    Render::SetVertexFormat( mVertexFormat );
    Render::SetShader( mShader );
    Render::SetBlendState( mBlendState );
    Render::SetDepthState( mDepthState );
    Render::SetRasterizerState( mRasterizerState );
    Render::SetSamplerState( mSamplerState );
    Render::SetTexture( { cubemap } );
    Render::Submit( viewId, TAC_STACK_FRAME );
  }
}
