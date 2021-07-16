#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacCamera.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/light/taclight.h"
#include "src/space/model/tacModel.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacShadowPresentation.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"


namespace Tac
{
  static Render::ShaderHandle sShader;

  struct ShadowModelVisitor : public ModelVisitor
  {
    void operator()( Model* model ) override
    {
      Errors errors;
      Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                           model->mModelIndex,
                                                           mVertexDeclarations,
                                                           errors );
      if( !mesh )
        return;

      DefaultCBufferPerObject objBuf;
      objBuf.Color = { model->mColorRGB, 1 };
      objBuf.World = model->mEntity->mWorldTransform;
      Render::UpdateConstantBuffer( mObjConstantBuffer,
                                    &objBuf,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );

      for( const SubMesh& subMesh : mesh->mSubMeshes )
      {
        Render::BeginGroup( subMesh.mName, TAC_STACK_FRAME );
        Render::SetShader( sShader );
        Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
        Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
        Render::SetBlendState( mBlendState );
        Render::SetRasterizerState( mRasterizerStateHandle );
        Render::SetSamplerState( mSamplerState );
        Render::SetDepthState( mDepthState );
        Render::SetVertexFormat( mVertexFormatHandle );

        // going to use the depth buffer solely, dont need a color buffer
        //Render::SetTexture( { mShadowColor } );
        //
        // the fbo depth texture is alreay bound for pixel output, cannot bind it as an
        // input as a shader resource view
        //Render::SetTexture( { mShadowDepth } );
        //

        Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
        Render::Submit( mViewHandle, TAC_STACK_FRAME );
        Render::EndGroup( TAC_STACK_FRAME );

      }
    }

    Render::ViewHandle            mViewHandle;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::SamplerStateHandle    mSamplerState;
    Render::ConstantBufferHandle  mObjConstantBuffer;
    //Render::TextureHandle         mShadowColor;
    Render::TextureHandle         mShadowDepth;
    Render::VertexFormatHandle    mVertexFormatHandle;
    Render::VertexDeclarations    mVertexDeclarations;
    Render::RasterizerStateHandle mRasterizerStateHandle;
  };

  static Render::TextureHandle CreateShadowMapDepth( const Light* light )
  {
    Render::TexSpec texSpecDepth;
    texSpecDepth.mImage.mFormat.mElementCount = 1;
    texSpecDepth.mImage.mFormat.mPerElementByteCount = sizeof( uint16_t );
    texSpecDepth.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
    texSpecDepth.mImage.mWidth = light->mShadowResolution;
    texSpecDepth.mImage.mHeight = light->mShadowResolution;
    texSpecDepth.mBinding = Render::Binding::DepthStencil | Render::Binding::ShaderResource;
    Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( textureHandleDepth, "shadowmap-depth" );
    return textureHandleDepth;

  }

  //static Render::TextureHandle CreateShadowMapColor( const Light* light )
  //{
  //    Render::TexSpec texSpecColor;
  //    texSpecColor.mImage.mFormat.mElementCount = 4;
  //    texSpecColor.mImage.mFormat.mPerElementByteCount = 1;
  //    texSpecColor.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
  //    texSpecColor.mImage.mWidth = light->mShadowResolution;
  //    texSpecColor.mImage.mHeight = light->mShadowResolution;
  //    texSpecColor.mBinding = Render::Binding::ShaderResource | Render::Binding::RenderTarget;
  //    Render::TextureHandle textureHandleColor = Render::CreateTexture( texSpecColor, TAC_STACK_FRAME );
  //    Render::SetRenderObjectDebugName( textureHandleColor, "shadowmap-color" );
  //    return textureHandleColor;
  //}

  static void CreateShadowMapFramebuffer( Light* light )
  {
    //Render::TextureHandle textureHandleColor = CreateShadowMapColor( light );
    Render::TextureHandle textureHandleDepth = CreateShadowMapDepth( light );
    Render::FramebufferTextures framebufferTextures = { /*textureHandleColor,*/ textureHandleDepth };
    Render::FramebufferHandle framebufferHandle
      = Render::CreateFramebufferForRenderToTexture( framebufferTextures, TAC_STACK_FRAME );

    light->mShadowFramebuffer = framebufferHandle;
    light->mShadowMapDepth = textureHandleDepth;
    //light->mShadowMapColor = textureHandleColor;
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
    DefaultCBufferPerFrame GetPerFrameData( const Light* light )
    {


      Camera my_camera = light->GetCamera();



      Camera* camera = &my_camera;

      float a;
      float b;
      Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                          camera->mNearPlane,
                                          a,
                                          b );
      const float w = ( float )light->mShadowResolution;
      const float h = ( float )light->mShadowResolution;
      const float aspect = w / h;
      const m4 view = camera->View();
      const m4 proj = camera->Proj( a, b, aspect );
      const double elapsedSeconds = ShellGetElapsedSeconds();

      //{
      //  v4 posNear = { 0,0,-camera->mNearPlane,1 };
      //  v4 posClip = proj * posNear;
      //  v3 posNdc = posClip.xyz() / posClip.w;
      //  ++asdf;
      //}

      //{
      //  v4 posView = { 0,0,-camera->mFarPlane,1 };
      //  v4 posClip = proj * posView;
      //  v3 posNDC = posClip.xyz() / posClip.w;
      //  ++asdf;
      //}

      DefaultCBufferPerFrame perFrameData;
      perFrameData.mFar = camera->mFarPlane;
      perFrameData.mNear = camera->mNearPlane;
      perFrameData.mView = view;
      perFrameData.mProjection = proj;
      perFrameData.mGbufferSize = { w, h };
      perFrameData.mSecModTau = ( float )std::fmod( elapsedSeconds, 6.2831853 );
      return perFrameData;
    }

    void operator()( Light* light ) override
    {
      if( !light->mCastsShadows )
        return;
      TAC_RENDER_GROUP_BLOCK( FrameMemoryPrintf( "Light Shadow %p", light ) );
      CreateShadowMapResources( light );

      const DefaultCBufferPerFrame perFrameData = GetPerFrameData( light );


      auto cam = light->GetCamera();
      auto draw = light->mEntity->mWorld->mDebug3DDrawData;
      m4 world_xform = light->mEntity->mWorldTransform;
      draw->DebugDraw3DArrow( cam.mPos, cam.mPos + cam.mRight, { 1,0,0 } );
      draw->DebugDraw3DArrow( cam.mPos, cam.mPos + cam.mUp, { 0,1,0 } );
      draw->DebugDraw3DArrow( cam.mPos, cam.mPos + -cam.mForwards, { 0,0,1 } );

      if( 0 )
      {

        const char* r0 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m00, world_xform.m01, world_xform.m02 );
        const char* r1 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m10, world_xform.m11, world_xform.m12 );
        const char* r2 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m20, world_xform.m21, world_xform.m22 );

        ImGuiSetNextWindowSize( v2( 1, 1 ) * 300 );
        ImGuiBegin( "test" );
        ImGuiText( "world xform" );
        ImGuiText( r0 );
        ImGuiText( r1 );
        ImGuiText( r2 );
        ImGuiEnd();
      }


      Render::SetViewFramebuffer( light->mShadowView, light->mShadowFramebuffer );
      Render::SetViewport( light->mShadowView, Render::Viewport( light->mShadowResolution,
                                                                 light->mShadowResolution ) );
      Render::SetViewScissorRect( light->mShadowView, Render::ScissorRect( light->mShadowResolution,
                                                                           light->mShadowResolution ) );
      Render::UpdateConstantBuffer( GamePresentationGetPerFrame(),
                                    &perFrameData,
                                    sizeof( DefaultCBufferPerFrame ),
                                    TAC_STACK_FRAME );


      ShadowModelVisitor modelVisitor;
      modelVisitor.mViewHandle = light->mShadowView;
      modelVisitor.mDepthState = GamePresentationGetDepthState();
      modelVisitor.mBlendState = GamePresentationGetBlendState();
      modelVisitor.mSamplerState = GamePresentationGetSamplerState();
      modelVisitor.mObjConstantBuffer = GamePresentationGetPerObj();
      modelVisitor.mVertexDeclarations = GamePresentationGetVertexDeclarations();
      modelVisitor.mVertexFormatHandle = GamePresentationGetVertexFormat();
      modelVisitor.mRasterizerStateHandle = GamePresentationGetRasterizerState();
      modelVisitor.mShadowDepth = light->mShadowMapDepth;
      //modelVisitor.mShadowColor = light->mShadowMapColor;
      graphics->VisitModels( &modelVisitor );
    }

    Graphics* graphics;
  };



  void        ShadowPresentationInit( Errors& errors )
  {
    const Render::ConstantBuffers constantBuffers =
    {
      GamePresentationGetPerFrame(),
      GamePresentationGetPerObj(),
    };

    sShader = Render::CreateShader( Render::ShaderSource::FromPath( "Shadow" ), constantBuffers, TAC_STACK_FRAME );
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

    //Render::SubmitFrame();
    //Render::SubmitFrame();
    //Render::SubmitFrame();

  }

  void        ShadowPresentationDebugImGui( Graphics* graphics )
  {
    if( !ImGuiCollapsingHeader( "Shadow Presentation" ) )
      return;
  }
}
