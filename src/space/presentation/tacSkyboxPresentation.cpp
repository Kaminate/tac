#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/string/tacString.h"
#include "src/common/tacMemory.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/profile/tacProfile.h"

namespace Tac
{

  static Render::VertexFormatHandle    mVertexFormat;
  static Render::ShaderHandle          mShader;
  static Render::ConstantBufferHandle  mPerFrame;
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
    Render::DestroyConstantBuffer( mPerFrame, TAC_STACK_FRAME );
  }

  void SkyboxPresentationInit( Errors& errors )
  {

    mPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mPerFrame, "skybox-per-frame" );

    mShader = Render::CreateShader( Render::ShaderSource::FromPath( "Skybox" ),
                                    Render::ConstantBuffers{ mPerFrame },
                                    TAC_STACK_FRAME );

    Render::VertexDeclaration pos;
    pos.mAlignedByteOffset = 0;
    pos.mAttribute = Render::Attribute::Position;
    pos.mTextureFormat.mElementCount = 3;
    pos.mTextureFormat.mPerElementByteCount = sizeof( float );
    pos.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    mVertexDecls = Render::VertexDeclarations{ pos };

    mVertexFormat = Render::CreateVertexFormat( mVertexDecls,
                                                mShader,
                                                TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mVertexFormat, "skybox-fmt" );
    TAC_HANDLE_ERROR( errors );


    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = Render::BlendConstants::One;
    blendStateData.mDstRGB = Render::BlendConstants::Zero;
    blendStateData.mBlendRGB = Render::BlendMode::Add;
    blendStateData.mSrcA = Render::BlendConstants::Zero;
    blendStateData.mDstA = Render::BlendConstants::One;
    blendStateData.mBlendA = Render::BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "skybox-blend" );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = Render::DepthFunc::LessOrEqual;
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "skybox-depth" );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None; // todo
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState, "skybox-rast" );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Render::Filter::Linear;
    mSamplerState = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState, "skybox-samp" );
    TAC_HANDLE_ERROR( errors );
  }

  void SkyboxPresentationRender( const Camera* camera,
                                 const int viewWidth,
                                 const int viewHeight,
                                 const Render::ViewHandle viewId,
                                 const StringView skyboxDir )
  {
    /*TAC_PROFILE_BLOCK*/;
    const StringView defaultSkybox = "assets/skybox/daylight";
    const StringView skyboxDirToUse = skyboxDir.empty() ? defaultSkybox : skyboxDir;
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
    DefaultCBufferPerFrame perFrame;
    perFrame.mFar = camera->mFarPlane;
    perFrame.mNear = camera->mNearPlane;
    perFrame.mGbufferSize = { ( float )viewWidth, ( float )viewHeight };
    perFrame.mView = view;
    perFrame.mProjection = camera->Proj( a, b, aspect );
    const SubMesh* subMesh = &mesh->mSubMeshes[ 0 ];
    Render::UpdateConstantBuffer( mPerFrame, &perFrame, sizeof( DefaultCBufferPerFrame ), TAC_STACK_FRAME );
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
