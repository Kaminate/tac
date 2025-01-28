#include "tac_jppt_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac-ecs/presentation/jpptpresentation/tac_jppt_BVH.h"
#include "tac-ecs/entity/tac_entity.h"


#if TAC_JPPT_PRESENTATION_ENABLED()

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  static bool                 sInitialized;
  static bool                 sEnabled;
  static Render::BufferHandle sCameraGPUBuffer;
  static SceneBVH*            sSceneBvh;
  static SceneBVHDebug        sSceneBvhDebug;
  static bool                 sShouldCreateSceneBVH;
  static bool                 sShouldRenderSceneBVHCPU;
  static bool                 sShouldRenderSceneBVHGPU;
  static Errors               createBVHErrors;
  static bool                 sVisualizePositions;
  static bool                 sVisualizeNormals;
  static float                sVisualizeNormalLength{ 1.0f };
  static bool                 sVisualizeFrame;
  static int                  sVisualizeFrameIndex;
  static const char*          sWindowName{ "JPPT" };
  static Errors               getMeshErrors;

  static int                      sTextureW;
  static int                      sTextureH;
  static Render::TextureHandle    sTexture;
  static Render::ProgramHandle    sProgram;
  static Render::PipelineHandle   sPipeline;

  struct JPPTCamera
  {
    m4    mFrame        { 1 };
    float mLens         { 0.05f }; // ?
    float mFilm         { 0.036f }; // ?
    float mAspect       { 1.5f };
    float mFocus        { 1000 }; // ?
    v3    mPad0         {};
    float mAperture     {};
    i32   mOrthographic {};
    v3    mPad1         {};
  };
  // ^ wat is with this weird padding


  //struct JPPTVertex
  //{
  //  v3 mPos;
  //  v2 mTexCoord;
  //  v4 mColor;
  //};

  static void VisualizeNormal( Entity* entity )
  {
    if( !sVisualizeNormals )
      return;

    if( getMeshErrors )
      return;

    Model* model{ ( Model* )entity->GetComponent( Model().GetEntry() ) };
    if( !model )
      return;

    Debug3DDrawData* drawData{ entity->mWorld->mDebug3DDrawData };

    const ModelAssetManager::Params getMeshParams
    {
      .mPath        { model->mModelPath },
      .mModelIndex  { model->mModelIndex },
    };
    Mesh* mesh { ModelAssetManager::GetMesh( getMeshParams, getMeshErrors ) };
    if( !mesh )
      return;

    if( getMeshErrors )
      return;

    const m4 world{ model->mEntity->mWorldTransform };

    for( const int i : mesh->mJPPTCPUMeshData.mIndexes )
    {
      const v3 p_model{ mesh->mJPPTCPUMeshData.mPositions[ i ] };
      const v3 p_world{ ( world * v4( p_model, 1.0f ) ).xyz() };
      const v3 n_model{ mesh->mJPPTCPUMeshData.mNormals[ i ] };
      const v3 n_world{ ( world * v4( n_model, 0.0f ) ).xyz() };

      drawData->DebugDraw3DLine( p_world,
                                 p_world + n_world * sVisualizeNormalLength,
                                 v4( 0, 0, 1, 1 ) );
    }
  }

  static void VisualizeFrame( Entity* entity )
  {
    if( !sVisualizeFrame )
      return;

    if( getMeshErrors )
      return;

    Debug3DDrawData* drawData{ entity->mWorld->mDebug3DDrawData };

    Model* model{ ( Model* )entity->GetComponent( Model().GetEntry() ) };
    if( !model )
      return;

    const ModelAssetManager::Params getMeshParams
    {
      .mPath        { model->mModelPath },
      .mModelIndex  { model->mModelIndex },
    };
    Mesh* mesh { ModelAssetManager::GetMesh( getMeshParams, getMeshErrors ) };
    if( !mesh )
      return;

    if( getMeshErrors )
      return;

    const m4 worldMatrix{ model->mEntity->mWorldTransform };

    const JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };

    const int ii{ sVisualizeFrameIndex };
    if( ii < 0 || ii >= jpptCPUMeshData.mIndexes.size() )
      return;

    const int i{ jpptCPUMeshData.mIndexes[ ii ] };
    const bool validPos{ i >= 0 && i < jpptCPUMeshData.mPositions.size() };
    if( !validPos )
      return;

    const v3 p_model{ jpptCPUMeshData.mPositions[ i ] };
    const v3 p_world{ ( worldMatrix * v4( p_model, 1 ) ).xyz() };

    if( const bool validNor{ i >= 0 && i < jpptCPUMeshData.mNormals.size() } )
    {
      const v3 n_model{ jpptCPUMeshData.mNormals[ i ] };
      const v3 n_world{ ( worldMatrix * v4( n_model, 0 ) ).xyz() };

      drawData->DebugDraw3DLine( p_world,
                                                 p_world + n_world * sVisualizeNormalLength,
                                                 v4( 0, 0, 1, 1 ) );
    }

    if( const bool validTan{ i >= 0 && i < jpptCPUMeshData.mTangents.size() })
    {
      const v3 t_model{ jpptCPUMeshData.mTangents[ i ] };
      const v3 t_world{ ( worldMatrix * v4( t_model, 0 ) ).xyz() };

      drawData->DebugDraw3DLine( p_world,
                                                 p_world + t_world * sVisualizeNormalLength,
                                                 v4( 1, 0, 0, 1 ) );
    }

    if( const bool validBit{ i >= 0 && i < jpptCPUMeshData.mBitangents.size() } )
    {
      const v3 b_model{ jpptCPUMeshData.mBitangents[ i ] };
      const v3 b_world{ ( worldMatrix * v4( b_model, 0 ) ).xyz() };
      drawData->DebugDraw3DLine( p_world,
                                 p_world + b_world * sVisualizeNormalLength,
                                 v4( 0, 1, 0, 1 ) );
    }
  }

  static void VisualizePosition( Entity* entity )
  {
    if( !sVisualizePositions )
      return;

    if( getMeshErrors )
      return;

    Model* model{ ( Model* )entity->GetComponent( Model().GetEntry() ) };
    if( !model )
      return;

    const ModelAssetManager::Params getMeshParams
    {
      .mPath        { model->mModelPath },
      .mModelIndex  { model->mModelIndex },
    };
    Mesh* mesh { ModelAssetManager::GetMesh( getMeshParams, getMeshErrors ) };
    if( !mesh )
      return;

    if( getMeshErrors )
      return;

    const m4 worldMatrix{ model->mEntity->mWorldTransform };

    for( int ii{}; ii <  mesh->mJPPTCPUMeshData.mIndexes.size(); ii += 3 )
    {
      const int i0{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 0 ] };
      const int i1{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 1 ] };
      const int i2{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 2 ] };
      const v3 p0_model{ mesh->mJPPTCPUMeshData.mPositions[ i0 ] };
      const v3 p1_model{ mesh->mJPPTCPUMeshData.mPositions[ i1 ] };
      const v3 p2_model{ mesh->mJPPTCPUMeshData.mPositions[ i2 ] };

      const v3 p0_world{ ( worldMatrix * v4( p0_model, 1.0f ) ).xyz() };
      const v3 p1_world{ ( worldMatrix * v4( p1_model, 1.0f ) ).xyz() };
      const v3 p2_world{ ( worldMatrix * v4( p2_model, 1.0f ) ).xyz() };


      entity->mWorld->mDebug3DDrawData->DebugDraw3DTriangle( p0_world, p1_world, p2_world );
    }
  }

  static void Visualize( const World* world )
  {
    for( Entity* entity : world->mEntities )
    {
      VisualizePosition( entity );
      VisualizeNormal( entity );
      VisualizeFrame( entity );
    }
  }

  static void CreateTexture( Errors& errors )
  {
    const WindowHandle windowHandle{ ImGuiGetWindowHandle( sWindowName ) };
    TAC_ASSERT( windowHandle.IsValid() );
    const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyTexture( sTexture );

    // kRGBA8_unorm_srgb cannot be used with typed uav
    const Render::TexFmt fmt{ Render::TexFmt::kRGBA8_unorm };

    sTextureW = windowSize.x;
    sTextureH = windowSize.y;

    const Render::Image image
    {
      .mWidth   { sTextureW },
      .mHeight  { sTextureH },
      .mDepth   { 1 },
      .mFormat  { fmt },
    };

    const Render::Binding binding
    {
      Render::Binding::ShaderResource | // ImGuiImage input 
      Render::Binding::UnorderedAccess // compute shader output 
    };

    const Render::CreateTextureParams createTextureParams
    {
      .mImage                  { image },
      .mMipCount               { 1 },
      .mBinding                { binding },
      .mUsage                  { Render::Usage::Default },
      .mCpuAccess              { Render::CPUAccess::None },
      .mOptionalName           { "JPPT" }
    };
    TAC_CALL( sTexture = renderDevice->CreateTexture( createTextureParams, errors ) );
  }

  static void CreatePipeline( Errors& errors )
  {
    if( sPipeline.IsValid() )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs{ "jppt" },
    };

    TAC_CALL( sProgram = renderDevice->CreateProgram( programParams, errors ) );
    const Render::PipelineParams pipelineParams
    {
      .mProgram{ sProgram },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    Render::IShaderVar* outputTexture {
      renderDevice->GetShaderVariable( sPipeline, "sOutputTexture" ) };
    outputTexture->SetResource( sTexture );
  }

  static void RenderSceneGPU( Errors& errors )
  {
    if( !sShouldRenderSceneBVHGPU )
      return;
    sShouldRenderSceneBVHGPU = false;
    TAC_CALL( CreateTexture( errors ) );
    TAC_CALL( CreatePipeline( errors ) );

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    //const Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( windowHandle ) };
    //const Render::TextureHandle swapChainColor{
      //renderDevice->GetSwapChainCurrentColor( swapChain ) };
    //const Render::TextureHandle swapChainDepth{
    //  renderDevice->GetSwapChainDepth( swapChain ) };
    TAC_CALL( Render::IContext::Scope renderContextScope{
      renderDevice->CreateRenderContext( errors ) } );

    Render::IContext* renderContext{ renderContextScope.GetContext() };
    //const Render::Targets renderTargets
    //{
    //  .mColors { swapChainColor },
    //  .mDepth  { swapChainDepth },
    //};

    //const float t{ ( float )Sin( renderParams.mTimestamp.mSeconds * 2.0 ) * 0.5f + 0.5f };

    const v3i threadGroupCounts( RoundUpToNearestMultiple( sTextureW, 8 ),
                                 RoundUpToNearestMultiple( sTextureH, 8 ),
                                 1 );

    //renderContext->SetRenderTargets( renderTargets );
    //renderContext->ClearColor( swapChainColor, v4( t, 0, 1, 1 ) );

    renderContext->SetPipeline( sPipeline );
    renderContext->CommitShaderVariables();
    renderContext->Dispatch( threadGroupCounts );
    renderContext->SetSynchronous(); // imgui happens after

    TAC_CALL( renderContext->Execute( errors ) );
  }

  /*
    uvec2 GlobalID = GLOBAL_ID();
    if (GlobalID.x < Width && GlobalID.y < Height) {

        vec2 UV = vec2(GLOBAL_ID()) / vec2(ImageSize);
        ray Ray = GetRay(UV);

FN_DECL ray GetRay(vec2 ImageUV)
{
    camera Camera = Cameras[0];

    // Point on the film
    vec3 Q = vec3(
        (0.5f - ImageUV.x),
        (ImageUV.y - 0.5f),
        1
    );
    vec3 RayDirection = -normalize(Q);
    vec3 PointOnLens = vec3 (0,0,0);

    //Transform the ray direction and origin
    ray Ray = MakeRay(
        TransformPoint(Camera.Frame, PointOnLens),
        TransformDirection(Camera.Frame, RayDirection),
        vec3(0)
    );
    return Ray;
}
*/

  struct JPPTCPURaytrace
  {
    void Run( const Camera* camera )
    {
      mPixels.resize( mWidth * mHeight );

      const float aspect{ ( float )mWidth / ( float )mHeight };
      const float halfFovYRad{ camera->mFovyrad / 2.0f };
      const float halfNearPlaneHeight { camera->mNearPlane * Tan( halfFovYRad ) };
      const float halfNearPlaneWidth { aspect * halfNearPlaneHeight };

      for( int iRow{}; iRow < mHeight; ++iRow )
      {
        for( int iCol{}; iCol < mWidth; ++iCol )
        {
          // directx style uvs
          const float u { ( iCol + 0.5f ) / mWidth };
          const float v { ( iRow + 0.5f ) / mHeight };

          const int iPixel{ iCol + iRow * mWidth };

          const float ndcX{ ( 0 + u ) * 2 - 1 };
          const float ndcY{ ( 1 - v ) * 2 - 1 };

          const v3 pointOnFilm_worldspace{
            camera->mForwards * camera->mNearPlane +
            camera->mRight * halfNearPlaneWidth +
            camera->mUp * halfNearPlaneHeight };

          const TLAS& tlas{ sSceneBvh->mTLAS };

          const BVHRay bvhRay_worldspace
          {
            .mOrigin    { camera->mPos },
            .mDirection { pointOnFilm_worldspace - camera->mPos },
          };

          const SceneIntersection sceneIntersection{
            sSceneBvh->IntersectTLAS( bvhRay_worldspace ) };

          mPixels[ iPixel ] = PixelRGBA8Unorm::SetColor4( v4( u, v, 0, 1 ) );
          mPixels[ iPixel ].r += ( u8 )( ( sceneIntersection.mDistance / 100.0f ) * 255.0f );
        }
      }
    }

    struct PixelRGBA8Unorm
    {
      static PixelRGBA8Unorm SetColor4( v4 rgba )
      {
        return PixelRGBA8Unorm
        {
          .r{ ( u8 )( Saturate( rgba.x ) * 255.0f ) },
          .g{ ( u8 )( Saturate( rgba.y ) * 255.0f ) },
          .b{ ( u8 )( Saturate( rgba.z ) * 255.0f ) },
          .a{ ( u8 )( Saturate( rgba.w ) * 255.0f ) },
        };
      }

      static PixelRGBA8Unorm SetColor3( v3 rgb )
      {
        return PixelRGBA8Unorm
        {
          .r{ ( u8 )( Saturate( rgb.x ) * 255.0f ) },
          .g{ ( u8 )( Saturate( rgb.y ) * 255.0f ) },
          .b{ ( u8 )( Saturate( rgb.z ) * 255.0f ) },
          .a{ 255 },
        };
      }
      u8 r{};
      u8 g{};
      u8 b{};
      u8 a{};
    };

    Vector< PixelRGBA8Unorm > mPixels;
    int mWidth {};
    int mHeight {};
  };

  static void RenderSceneCPU( Errors& errors, const Camera* camera )
  {
    if( !sShouldRenderSceneBVHCPU )
      return;
    sShouldRenderSceneBVHCPU = false;

    const WindowHandle windowHandle{ ImGuiGetWindowHandle( sWindowName ) };
    TAC_ASSERT( windowHandle.IsValid() );
    const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyTexture( sTexture );

    sTextureW = windowSize.x;
    sTextureH = windowSize.y;

    const Render::Image image
    {
      .mWidth   { sTextureW },
      .mHeight  { sTextureH },
      .mDepth   { 1 },
      .mFormat  { Render::TexFmt::kRGBA8_unorm },
    };

    const Render::Binding binding
    {
      Render::Binding::ShaderResource
    };

    JPPTCPURaytrace cpuRaytrace;
    cpuRaytrace.mWidth = sTextureW;
    cpuRaytrace.mHeight = sTextureH;
    cpuRaytrace.Run( camera );

    const Render::CreateTextureParams::Subresource subresource
    {
      .mBytes{ cpuRaytrace.mPixels.data() },
      .mPitch{ 4 * sTextureW },
    };

    const Render::CreateTextureParams createTextureParams
    {
      .mImage                  { image },
      .mMipCount               { 1 },
      .mSubresources           { &subresource },
      .mBinding                { binding },
      .mUsage                  { Render::Usage::Default },
      .mCpuAccess              { Render::CPUAccess::None },
      .mOptionalName           { "JPPT" }
    };
    TAC_CALL( sTexture = renderDevice->CreateTexture( createTextureParams, errors ) );
  }

  void             JPPTPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;


    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::CreateBufferParams bufferParams
    {
      .mByteCount     { sizeof( JPPTCamera ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ShaderResource },
      .mOptionalName  { "jpptcamera" },
    };
    TAC_CALL( sCameraGPUBuffer = renderDevice->CreateBuffer( bufferParams, errors ) );



    sInitialized = true;
  }

  void             JPPTPresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void             JPPTPresentation::Render( Render::IContext* renderContext,
                                             const World* world,
                                             const Camera* camera,
                                             const v2i viewSize,
                                             const Render::TextureHandle dstColorTex,
                                             const Render::TextureHandle dstDepthTex,
                                             Errors& errors )
  {
    TAC_ASSERT( sInitialized );
    if( !sEnabled )
      return;


    if( sShouldCreateSceneBVH )
    {
      TAC_DELETE sSceneBvh;
      sSceneBvh = SceneBVH::CreateBVH( world, createBVHErrors );
      sShouldCreateSceneBVH = false;
    }

    TAC_CALL( RenderSceneCPU( errors, camera ) );
    TAC_CALL( RenderSceneGPU( errors ) );



    Visualize( world );
    sSceneBvhDebug.DebugVisualizeSceneBVH( world->mDebug3DDrawData, sSceneBvh );
  }

  void             JPPTPresentation::DebugImGui()
  {
    ImGuiCheckbox( "JPPT Presentation Enabled", &sEnabled );
    if( !sEnabled )
      return;

    ImGuiSetNextWindowDisableBG();
    if( !ImGuiBegin( sWindowName ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd() );

    if( sTexture.IsValid() )
    {
      const WindowHandle windowHandle{ ImGuiGetWindowHandle() };
      const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };
      const v2 cursorPos{ ImGuiGetCursorPos() };
      ImGuiSetCursorPos( {} );
      ImGuiImage( sTexture.GetIndex(), windowSize );
      ImGuiSetCursorPos( cursorPos );
    }

    if( sSceneBvh )
    {
      if( ImGuiButton( "Destroy Scene BVH" ) )
      {
        TAC_DELETE sSceneBvh;
        sSceneBvh = nullptr;
      }

      sShouldRenderSceneBVHCPU |= ImGuiButton( "Render Scene (CPU)" );
      sShouldRenderSceneBVHGPU |= ImGuiButton( "Render Scene (GPU)" );
    }
    else
      sShouldCreateSceneBVH |= ImGuiButton( "Create Scene BVH" );

    sSceneBvhDebug.DebugImguiSceneBVH( sSceneBvh );

    if( getMeshErrors )
    {
      ImGuiText( "Get mesh errors: " + getMeshErrors.ToString() );
      if( ImGuiButton( "Clear get mesh errors" ) )
        getMeshErrors = {};
    }
    ImGuiCheckbox( "Visualize Positions", &sVisualizePositions );
    if( ImGuiCheckbox( "Visualize Normals", &sVisualizeNormals ) && sVisualizeNormals )
      sVisualizeFrame = false;
    if( sVisualizeNormals || sVisualizeFrame )
      ImGuiDragFloat( "Normal Length", &sVisualizeNormalLength );

    if( ImGuiCheckbox( "Visualize Frame", &sVisualizeFrame ) && sVisualizeFrame )
      sVisualizeNormals = false;

    if( sVisualizeFrame )
      ImGuiDragInt( "Visualize Frame Index", &sVisualizeFrameIndex );

    if( createBVHErrors )
      ImGuiText( createBVHErrors.ToString() );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac

#endif // TAC_JPPT_PRESENTATION_ENABLED
