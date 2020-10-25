#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/common/tacString.h"
#include "src/common/tacMemory.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/profile/tacProfile.h"

namespace Tac
{


  SkyboxPresentation::~SkyboxPresentation()
  {
    Render::DestroyShader( mShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( mVertexFormat, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mPerFrame, TAC_STACK_FRAME );
  }
  void SkyboxPresentation::Init( Errors& errors )
  {

    mPerFrame = Render::CreateConstantBuffer( "skybox per frame",
                                              sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mShader = Render::CreateShader( "skybox",
                                    Render::ShaderSource::FromPath( "Skybox" ),
                                    Render::ConstantBuffers( mPerFrame ),
                                    TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration pos;
    pos.mAlignedByteOffset = 0;
    pos.mAttribute = Attribute::Position;
    pos.mTextureFormat.mElementCount = 3;
    pos.mTextureFormat.mPerElementByteCount = sizeof( float );
    pos.mTextureFormat.mPerElementDataType = GraphicsType::real;

    mVertexFormat = Render::CreateVertexFormat( "skybox",
                                                Render::VertexDeclarations( pos ),
                                                mShader,
                                                TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mVertexDecls[ 0 ] = pos;

    Render::BlendState blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::Zero;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::Zero;
    blendStateData.dstA = BlendConstants::One;
    blendStateData.blendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( "skybox", blendStateData, TAC_STACK_FRAME );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = DepthFunc::LessOrEqual;
    mDepthState = Render::CreateDepthState( "skybox", depthStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None; // todo
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( "skybox", rasterizerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Linear;
    mSamplerState = Render::CreateSamplerState( "skybox", samplerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }
  void SkyboxPresentation::RenderSkybox( const int viewWidth,
                                         const int viewHeight,
                                         const Render::ViewHandle viewId,
                                         const StringView skyboxDir )
  {
    /*TAC_PROFILE_BLOCK*/;
    const StringView defaultSkybox = "assets/skybox/daylight";
    const StringView skyboxDirToUse = skyboxDir.empty() ? defaultSkybox : skyboxDir;
    Render::TextureHandle cubemap = TextureAssetManager::GetTextureCube( skyboxDirToUse,
                                                                         mGetSkyboxTextureErrors );
    TAC_ASSERT( mGetSkyboxTextureErrors.empty() );
    if( !cubemap.IsValid() )
      return;
    Mesh* mesh;
    ModelAssetManager::Instance->GetMesh( &mesh,
                                          "assets/editor/Box.gltf",
                                          mVertexFormat,
                                          mVertexDecls,
                                          kVertexFormatDeclCount,
                                          mGetSkyboxMeshErrors );
    TAC_ASSERT( mGetSkyboxMeshErrors.empty() );
    if( !mesh )
      return;
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );
    const float aspect = ( float )viewWidth / ( float )viewHeight;
    const m4 view = m4::ViewInv( v3( 0, 0, 0 ),
                               mCamera->mForwards,
                               mCamera->mRight,
                               mCamera->mUp );
    DefaultCBufferPerFrame perFrame;
    perFrame.mFar = mCamera->mFarPlane;
    perFrame.mNear = mCamera->mNearPlane;
    perFrame.mGbufferSize = { ( float )viewWidth, ( float )viewHeight };
    perFrame.mView = view;
    perFrame.mProjection = mCamera->Proj( a, b, aspect );
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
    Render::SetTexture( Render::DrawCallTextures( cubemap ) );
    Render::Submit( viewId, TAC_STACK_FRAME );
  }
}
