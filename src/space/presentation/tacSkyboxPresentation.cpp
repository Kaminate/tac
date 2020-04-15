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
    Renderer::Instance->RemoveRendererResource( mShader );
    Renderer::Instance->RemoveRendererResource( mVertexFormat );
    Renderer::Instance->RemoveRendererResource( mPerFrame );
  }
  void SkyboxPresentation::Init( Errors& errors )
  {

    CBufferData cBufferDataPerFrame = {};
    cBufferDataPerFrame.mName = "skybox per frame";
    cBufferDataPerFrame.mFrame = TAC_STACK_FRAME;
    cBufferDataPerFrame.shaderRegister = 0;
    cBufferDataPerFrame.byteCount = sizeof( DefaultCBufferPerFrame );
    Renderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
    TAC_HANDLE_ERROR( errors );

    ShaderData shaderData = {};
    shaderData.mShaderPath = "Skybox";
    shaderData.mCBuffers = { mPerFrame };
    shaderData.mFrame = TAC_STACK_FRAME;
    shaderData.mName = "skybox";
    Renderer::Instance->AddShader( &mShader, shaderData, errors );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration pos;
    pos.mAlignedByteOffset = 0;
    pos.mAttribute = Attribute::Position;
    pos.mTextureFormat.mElementCount = 3;
    pos.mTextureFormat.mPerElementByteCount = sizeof( float );
    pos.mTextureFormat.mPerElementDataType = GraphicsType::real;

    VertexFormatData vertexFormatData = {};
    vertexFormatData.mName = "skybox";
    vertexFormatData.mFrame = TAC_STACK_FRAME;
    vertexFormatData.shader = mShader;
    vertexFormatData.vertexFormatDatas = { pos };
    Renderer::Instance->AddVertexFormat( &mVertexFormat, vertexFormatData, errors );
    TAC_HANDLE_ERROR( errors );

    BlendStateData blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::Zero;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::Zero;
    blendStateData.dstA = BlendConstants::One;
    blendStateData.blendA = BlendMode::Add;
    blendStateData.mName = "skybox";
    blendStateData.mFrame = TAC_STACK_FRAME;
    Renderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
    TAC_HANDLE_ERROR( errors );

    DepthStateData depthStateData;
    depthStateData.depthTest = true;
    depthStateData.depthWrite = true;
    depthStateData.depthFunc = DepthFunc::LessOrEqual;
    depthStateData.mName = "skybox";
    depthStateData.mFrame = TAC_STACK_FRAME;
    Renderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
    TAC_HANDLE_ERROR( errors );

    RasterizerStateData rasterizerStateData;
    rasterizerStateData.cullMode = CullMode::None; // todo
    rasterizerStateData.fillMode = FillMode::Solid;
    rasterizerStateData.frontCounterClockwise = true;
    rasterizerStateData.mName = "skybox";
    rasterizerStateData.mFrame = TAC_STACK_FRAME;
    rasterizerStateData.multisample = false;
    rasterizerStateData.scissor = true;
    Renderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
    TAC_HANDLE_ERROR( errors );

    SamplerStateData samplerStateData;
    samplerStateData.mName = "skybox";
    samplerStateData.mFrame = TAC_STACK_FRAME;
    samplerStateData.filter = Filter::Linear;
    Renderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
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
    ModelAssetManager::Instance->GetMesh( &mesh, "assets/editor/Box.gltf", mVertexFormat, errors );
    TAC_ASSERT( errors.empty() );
    if( !mesh )
      return;

    static int i;
    ++i;

    float a;
    float b;
    Renderer::Instance->GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );
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
    Renderer::Instance->AddDrawCall( drawCallPerFrame );

    SubMesh* subMesh = &mesh->mSubMeshes[ 0 ];

    DrawCall2 drawCallGeometry = {};
    drawCallGeometry.mVertexBuffer = subMesh->mVertexBuffer;
    drawCallGeometry.mIndexBuffer = subMesh->mIndexBuffer;
    drawCallGeometry.mIndexCount = subMesh->mIndexBuffer->mIndexCount;
    drawCallGeometry.mVertexFormat = mVertexFormat;
    drawCallGeometry.mShader = mShader;
    drawCallGeometry.mBlendState = mBlendState;
    drawCallGeometry.mDepthState = mDepthState;
    drawCallGeometry.mPrimitiveTopology = PrimitiveTopology::TriangleList;
    drawCallGeometry.mRasterizerState = mRasterizerState;
    drawCallGeometry.mSamplerState = mSamplerState;
    drawCallGeometry.mFrame = TAC_STACK_FRAME;
    drawCallGeometry.mStartIndex = 0;
    drawCallGeometry.mTextures = { nullptr }; // { cubemap };
    drawCallGeometry.mRenderView = mDesktopWindow->mRenderView;
    Renderer::Instance->AddDrawCall( drawCallGeometry );

    Renderer::Instance->DebugBegin( "Skybox" );
    Renderer::Instance->RenderFlush();
    Renderer::Instance->DebugEnd();
  }
}
