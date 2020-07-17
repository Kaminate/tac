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
                                                Render::VertexDeclarations(pos),
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

    Render::CommandDataCreateDepthState depthStateData;
    depthStateData.depthTest = true;
    depthStateData.depthWrite = true;
    depthStateData.depthFunc = DepthFunc::LessOrEqual;
    mDepthState = Render::CreateDepthState( "skybox", depthStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::CommandDataCreateRasterizerState rasterizerStateData;
    rasterizerStateData.cullMode = CullMode::None; // todo
    rasterizerStateData.fillMode = FillMode::Solid;
    rasterizerStateData.frontCounterClockwise = true;
    rasterizerStateData.multisample = false;
    rasterizerStateData.scissor = true;
    mRasterizerState = Render::CreateRasterizerState( "skybox", rasterizerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::CommandDataCreateSamplerState samplerStateData;
    samplerStateData.filter = Filter::Linear;
    mSamplerState = Render::CreateSamplerState( "skybox", samplerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }
  void SkyboxPresentation::RenderSkybox( const String& skyboxDir )
  {
    /*TAC_PROFILE_BLOCK*/;
    static String defaultSkybox = "assets/skybox/daylight";
    const String& skyboxDirToUse = skyboxDir.empty() ? defaultSkybox : skyboxDir;

    Errors errors;
    Render::TextureHandle cubemap = TextureAssetManager::GetTextureCube( skyboxDirToUse, errors );
    TAC_ASSERT( errors.empty() );
    if( cubemap.mResourceId == Render::NullResourceId )
      return;

    Mesh* mesh;
    ModelAssetManager::Instance->GetMesh( &mesh,
                                          "assets/editor/Box.gltf",
                                          mVertexFormat,
                                          mVertexDecls,
                                          kVertexFormatDeclCount,
                                          errors );
    TAC_ASSERT( errors.empty() );
    if( !mesh )
      return;

    float a;
    float b;
    Render::GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );
    float aspect = ( float )mDesktopWindow->mWidth / ( float )mDesktopWindow->mHeight;

    DefaultCBufferPerFrame perFrame;
    perFrame.mFar = mCamera->mFarPlane;
    perFrame.mNear = mCamera->mNearPlane;
    perFrame.mGbufferSize = { ( float )mDesktopWindow->mWidth, ( float )mDesktopWindow->mHeight };
    perFrame.mView = M4ViewInv( v3( 0, 0, 0 ), mCamera->mForwards, mCamera->mRight, mCamera->mUp );
    perFrame.mProjection = mCamera->Proj( a, b, aspect );

    DrawCall2 drawCallPerFrame = {};
    drawCallPerFrame.mUniformDst = mPerFrame;
    drawCallPerFrame.mUniformSrcc = TemporaryMemoryFromT( perFrame );
    Render::AddDrawCall( drawCallPerFrame );

    SubMesh* subMesh = &mesh->mSubMeshes[ 0 ];

    DrawCall2 drawCallGeometry = {};
    drawCallGeometry.mVertexBuffer = subMesh->mVertexBuffer;
    drawCallGeometry.mIndexBuffer = subMesh->mIndexBuffer;
    drawCallGeometry.mIndexCount = subMesh->mIndexCount;
    drawCallGeometry.mVertexFormat = mVertexFormat;
    drawCallGeometry.mShader = mShader;
    drawCallGeometry.mBlendState = mBlendState;
    drawCallGeometry.mDepthState = mDepthState;
    drawCallGeometry.mPrimitiveTopology = PrimitiveTopology::TriangleList;
    drawCallGeometry.mRasterizerState = mRasterizerState;
    drawCallGeometry.mSamplerState = mSamplerState;
    drawCallGeometry.mFrame = TAC_STACK_FRAME;
    drawCallGeometry.mStartIndex = 0;
    drawCallGeometry.mTextureHandles = { cubemap };
    Render::AddDrawCall( drawCallGeometry );

    //Renderer::Instance->DebugBegin( "Skybox" );
    //Renderer::Instance->RenderFlush();
    //Renderer::Instance->DebugEnd();
  }
}
