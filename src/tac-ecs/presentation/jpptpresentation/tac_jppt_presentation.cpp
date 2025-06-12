#include "tac_jppt_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac-ecs/presentation/jpptpresentation/tac_jppt_BVH.h"
#include "tac-ecs/entity/tac_entity.h"


#if TAC_JPPT_PRESENTATION_ENABLED()

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  static bool                 sInitialized;
  static bool                 sEnabled{ true };
  static Render::BufferHandle sCameraGPUBuffer;
  static SceneBVH*            sSceneBvh;
  static SceneBVHDebug        sSceneBvhDebug;
  static bool                 sShouldCreateSceneBVH;
  static bool                 sShouldRenderSceneBVHCPU;
  static bool                 sShouldRenderSceneBVHGPU;
  static bool                 sVisualizeDistance;
  static Errors               createBVHErrors;
  static bool                 sVisualizePositions;
  static int                  sVisualizeSpeficicTriangle = -1;
  static bool                 sVisualizeModelSpace;
  static bool                 sVisualizeNormals;
  static float                sVisualizeNormalLength{ 1.0f };
  static bool                 sVisualizeFrame;
  static int                  sVisualizeFrameIndex;
  static const char*          sWindowName{ "JPPT" };
  static Errors               getMeshErrors;
  static struct
  {
    v2                 mUV            { .5f, .5f };
    Optional< Camera > mCamera        {};
    bool               mCaptureCamera {};
    bool               mEnabled       {};

    void DebugImgui()
    {
      if( !ImGuiCollapsingHeader( "CPU Ray Debug", ImGuiNodeFlags_DefaultOpen ) )
        return;
      TAC_IMGUI_INDENT_BLOCK;

      if( ImGuiButton( "Capture Camera" ) )
        mCaptureCamera = true;

      if( mCamera.HasValue() )
      {
        ImGuiCheckbox( "Enable Debug Render", &mEnabled );
        if( ImGuiDragFloat2( "uvs", mUV.data() ) )
        {
          mUV.x = Clamp( mUV.x, 0, 1 );
          mUV.y = Clamp( mUV.y, 0, 1 );
        }
      }
    }

    void DrawFrustum( float aspect, Debug3DDrawData* drawData )
    {
      if( !mEnabled )
        return;

      if(!mCamera.HasValue() )
        return;

      TAC_ASSERT( mCamera.HasValue() );
      Camera cam = mCamera.GetValue();
      Camera* camera = &cam;

      const float halfFovYRad{ camera->mFovyrad / 2.0f };

      const float halfNearPlaneHeight { camera->mNearPlane * Tan( halfFovYRad ) };
      const float halfNearPlaneWidth { aspect * halfNearPlaneHeight };
      const v3 nearPlaneX_worldspace{ camera->mRight * halfNearPlaneWidth };
      const v3 nearPlaneY_worldspace{ camera->mUp * halfNearPlaneHeight };
      const v3 nearPlane0_worldspace{ camera->mPos + camera->mForwards * camera->mNearPlane };

      const float halfFarPlaneHeight { camera->mFarPlane * Tan( halfFovYRad ) };
      const float halfFarPlaneWidth { aspect * halfFarPlaneHeight };
      const v3 farPlaneX_worldspace{ camera->mRight * halfFarPlaneWidth };
      const v3 farPlaneY_worldspace{ camera->mUp * halfFarPlaneHeight };
      const v3 farPlane0_worldspace{ camera->mPos + camera->mForwards * camera->mFarPlane };

      const v3 nearBL{ nearPlane0_worldspace - nearPlaneX_worldspace - nearPlaneY_worldspace };
      const v3 nearBR{ nearPlane0_worldspace + nearPlaneX_worldspace - nearPlaneY_worldspace };
      const v3 nearTL{ nearPlane0_worldspace - nearPlaneX_worldspace + nearPlaneY_worldspace };
      const v3 nearTR{ nearPlane0_worldspace + nearPlaneX_worldspace + nearPlaneY_worldspace };
      const v3 farBL{ farPlane0_worldspace - farPlaneX_worldspace - farPlaneY_worldspace };
      const v3 farBR{ farPlane0_worldspace + farPlaneX_worldspace - farPlaneY_worldspace };
      const v3 farTL{ farPlane0_worldspace - farPlaneX_worldspace + farPlaneY_worldspace };
      const v3 farTR{ farPlane0_worldspace + farPlaneX_worldspace + farPlaneY_worldspace };

      // draw frustum
      drawData->DebugDraw3DLine( camera->mPos, farBL );
      drawData->DebugDraw3DLine( camera->mPos, farBR );
      drawData->DebugDraw3DLine( camera->mPos, farTL );
      drawData->DebugDraw3DLine( camera->mPos, farTR );
      drawData->DebugDraw3DLine( nearBL, nearBR );
      drawData->DebugDraw3DLine( nearBR, nearTR );
      drawData->DebugDraw3DLine( nearTR, nearTL );
      drawData->DebugDraw3DLine( nearTL, nearBL );
      drawData->DebugDraw3DLine( farBL, farBR );
      drawData->DebugDraw3DLine( farBR, farTR );
      drawData->DebugDraw3DLine( farTR, farTL );
      drawData->DebugDraw3DLine( farTL, farBL );
    }

    void DrawRay(float aspect, Debug3DDrawData* drawData)
    {
      if(!sCPURayDebug.mEnabled)
        return;

      if( !sCPURayDebug.mCamera.HasValue() )
        return;

      if(! sSceneBvh )
        return;


      Camera cam{ mCamera.GetValue() };
      Camera* camera{&cam};
      const float halfFovYRad{ camera->mFovyrad / 2.0f };

      const float halfNearPlaneHeight{ camera->mNearPlane * Tan( halfFovYRad ) };
      const float halfNearPlaneWidth{ aspect * halfNearPlaneHeight };
      const v3 nearPlaneX_worldspace{ camera->mRight * halfNearPlaneWidth };
      const v3 nearPlaneY_worldspace{ camera->mUp * halfNearPlaneHeight };
      const v3 nearPlane0_worldspace{ camera->mPos + camera->mForwards * camera->mNearPlane };

      // directx style uvs
      const float u{ sCPURayDebug.mUV[ 0 ] };
      const float v{ sCPURayDebug.mUV[ 1 ] };

      const float ndcX{ ( 0 + u ) * 2 - 1 };
      const float ndcY{ ( 1 - v ) * 2 - 1 };

      const v3 pointOnFilm_worldspace{ nearPlane0_worldspace +
                                       nearPlaneX_worldspace * ndcX +
                                       nearPlaneY_worldspace * ndcY };
      const v3 rayUnitDir_worldspace{ Normalize( pointOnFilm_worldspace - camera->mPos ) };

      //const TLAS& tlas{ sSceneBvh->mTLAS };

      const BVHRay bvhRay
      {
        .mOrigin    { camera->mPos },
        .mDirection { rayUnitDir_worldspace },
      };

      const SceneIntersection sceneIntersection{
        sSceneBvh->IntersectTLAS( bvhRay ) };

      if( sceneIntersection.IsValid() )
      {
        drawData->DebugDraw3DLine( bvhRay.mOrigin,
                                   bvhRay.mOrigin +
                                   bvhRay.mDirection * sceneIntersection.mDistance );
      }
      else
      {
        if( camera->mNearPlane < 1 )
        {
          drawData->DebugDraw3DLine( camera->mPos,
                                     camera->mPos + Normalize( bvhRay.mDirection ) * 5 );
        }
        else
        {
          drawData->DebugDraw3DLine( camera->mPos, pointOnFilm_worldspace );
        }
      }
    }

    int GetDebugPx(int mWidth, int mHeight)
    {
      if( !sCPURayDebug.mEnabled )
        return -1;

      int iDebugPx{ -1 };
      // find the pixel closest to the debug uv
      float debugDist{};
      bool debugDistFound{};
      for( int iRow{}; iRow < mHeight; ++iRow )
      {
        for( int iCol{}; iCol < mWidth; ++iCol )
        {
          const int iPixel{ iCol + iRow * mWidth };
          // directx style uvs
          const float u { ( iCol + 0.5f ) / mWidth };
          const float v { ( iRow + 0.5f ) / mHeight };
          const v2 uv( u, v );
          const float dist{ Distance( uv, sCPURayDebug.mUV ) };
          if( !debugDistFound || dist < debugDist )
          {
            iDebugPx = iPixel;
            debugDistFound = true;
            debugDist = dist;
          }
        }
      }

      return iDebugPx;
    }

  } sCPURayDebug;

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

    const m4 transform{ sVisualizeModelSpace ? m4::Identity() : model->mEntity->mWorldTransform };

    for( const int i : mesh->mJPPTCPUMeshData.mIndexes )
    {
      const v3 p_model{ mesh->mJPPTCPUMeshData.mPositions[ i ] };
      const v3 p_world{ ( transform * v4( p_model, 1.0f ) ).xyz() };
      const v3 n_model{ mesh->mJPPTCPUMeshData.mNormals[ i ] };
      const v3 n_world{ ( transform * v4( n_model, 0.0f ) ).xyz() };

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

    const m4 transform{ sVisualizeModelSpace ? m4::Identity() : model->mEntity->mWorldTransform };

    const JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };

    const int ii{ sVisualizeFrameIndex };
    if( ii < 0 || ii >= jpptCPUMeshData.mIndexes.size() )
      return;

    const int i{ jpptCPUMeshData.mIndexes[ ii ] };
    const bool validPos{ i >= 0 && i < jpptCPUMeshData.mPositions.size() };
    if( !validPos )
      return;

    const v3 p_model{ jpptCPUMeshData.mPositions[ i ] };
    const v3 p_world{ ( transform * v4( p_model, 1 ) ).xyz() };

    if( const bool validNor{ i >= 0 && i < jpptCPUMeshData.mNormals.size() } )
    {
      const v3 n_model{ jpptCPUMeshData.mNormals[ i ] };
      const v3 n_world{ ( transform * v4( n_model, 0 ) ).xyz() };

      drawData->DebugDraw3DLine( p_world,
                                                 p_world + n_world * sVisualizeNormalLength,
                                                 v4( 0, 0, 1, 1 ) );
    }

    if( const bool validTan{ i >= 0 && i < jpptCPUMeshData.mTangents.size() })
    {
      const v3 t_model{ jpptCPUMeshData.mTangents[ i ] };
      const v3 t_world{ ( transform * v4( t_model, 0 ) ).xyz() };

      drawData->DebugDraw3DLine( p_world,
                                                 p_world + t_world * sVisualizeNormalLength,
                                                 v4( 1, 0, 0, 1 ) );
    }

    if( const bool validBit{ i >= 0 && i < jpptCPUMeshData.mBitangents.size() } )
    {
      const v3 b_model{ jpptCPUMeshData.mBitangents[ i ] };
      const v3 b_world{ ( transform * v4( b_model, 0 ) ).xyz() };
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

    const m4 transform{ sVisualizeModelSpace ? m4::Identity() : model->mEntity->mWorldTransform };

    for( int ii{}; ii <  mesh->mJPPTCPUMeshData.mIndexes.size(); ii += 3 )
    {
      if( sVisualizeSpeficicTriangle >= 0 )
        if( int iTri = ii / 3;
            iTri != sVisualizeSpeficicTriangle )
          continue;


      const int i0{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 0 ] };
      const int i1{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 1 ] };
      const int i2{ mesh->mJPPTCPUMeshData.mIndexes[ ii + 2 ] };
      const v3 p0_model{ mesh->mJPPTCPUMeshData.mPositions[ i0 ] };
      const v3 p1_model{ mesh->mJPPTCPUMeshData.mPositions[ i1 ] };
      const v3 p2_model{ mesh->mJPPTCPUMeshData.mPositions[ i2 ] };

      const v3 p0_world{ ( transform * v4( p0_model, 1.0f ) ).xyz() };
      const v3 p1_world{ ( transform * v4( p1_model, 1.0f ) ).xyz() };
      const v3 p2_world{ ( transform * v4( p2_model, 1.0f ) ).xyz() };


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
    v3 GetRadiance( v3 cameraPos_worldspace, v3 pointOnFilm_worldspace )
    {
      v3 prevColor{};

      int nSamples = 1;
      for( int iSample = 0; iSample < nSamples; ++iSample )
      {
        int nMaxBounces = 3;

        v3 radiance( 0 );
        v3 weight( 1 ); // is this related to "importance" / "potential function"?

        dynmc BVHRay bvhRay
        {
          .mOrigin    { cameraPos_worldspace },
          .mDirection { pointOnFilm_worldspace - cameraPos_worldspace },
        };

        for( int iBounce {}; iBounce < nMaxBounces; ++iBounce )
        {
          const TLAS& tlas{ sSceneBvh->mTLAS };


          const SceneIntersection sceneIntersection{
            sSceneBvh->IntersectTLAS( bvhRay ) };

          if( !sceneIntersection.IsValid() )
          {
            const v3 skyBlue{ HexToRGB( 0x87ceeb ) };
            radiance = v3( weight.x * skyBlue.x,
                           weight.y * skyBlue.y,
                           weight.z * skyBlue.z );
            break;
          }

          // debug code to visualize distance
          if( sVisualizeDistance && iBounce == 0 )
          {
            const AABB32 aabb{ tlas.Root().GetAABB() };
            const float maxDist{
              Distance( cameraPos_worldspace, aabb.Center() ) +
              Length( aabb.mMax - aabb.mMin ) };

            radiance = v3( Clamp( sceneIntersection.mDistance / maxDist, .0f, 1.f ) );
            break;
          }

          const BVHInstance& bvhInstance{
            sSceneBvh->mInstances.GetInstance( sceneIntersection.mInstanceIndex ) };

          const BVHMesh& bvhMesh{ 
            sSceneBvh->mMeshes.GetMesh( bvhInstance.mMeshIndex ) };

          const BVHTriangle& Tri{
            bvhMesh.mTriangles.GetTriangle( sceneIntersection.mPrimitiveIndex ) };

          const BVHTriangleExtraData& bvhTriExtra{
            bvhMesh.mTrianglesExtraData.GetTriangleExtraData(
              sceneIntersection.mPrimitiveIndex ) };

          const v3 v0{ Tri.mV0 };
          const v3 v1{ Tri.mV1 };
          const v3 v2{ Tri.mV2 };

          const v3 n0{ bvhTriExtra.mVertexExtraDatas[ 0 ].mNormal };
          const v3 n1{ bvhTriExtra.mVertexExtraDatas[ 1 ].mNormal };
          const v3 n2{ bvhTriExtra.mVertexExtraDatas[ 2 ].mNormal };

          const float u{ sceneIntersection.mU };
          const float v{ sceneIntersection.mV };
          const float w{ 1 - u - v };

          const v3 pos_modelspace{ v1 * u + v2 * v + v0 * w };
          const v3 pos_worldspace{
            ( bvhInstance.mTransform * v4( pos_modelspace, 1 ) ).xyz() };

          const v3 nor_modelspace{ n1 * u + n2 * v + n0 * w };
          const v3 nor_worldspace{
            Normalize( ( bvhInstance.mNormalTransform * v4( nor_modelspace, 0 ) ).xyz() ) };

          const v3 color{ bvhInstance.mMaterial->mColor.xyz() };
          weight[ 0 ] *= color[ 0 ];
          weight[ 1 ] *= color[ 1 ];
          weight[ 2 ] *= color[ 2 ];

          TAC_ASSERT_UNIMPLEMENTED;
          v3 incoming;

          bvhRay = BVHRay
          {
            .mOrigin    { pos_worldspace },
            .mDirection { incoming },
          };

          // when would this happen?
          if( weight == v3() )
            break;
        }

        const float sampleWeight{ 1.f / ( 1.f + iSample ) };
        prevColor = Lerp( prevColor, radiance, sampleWeight );
      }

      return prevColor;
    }

    void Run( const Camera* worldCamera, Debug3DDrawData* )
    {
      Camera cam1{ *worldCamera };
      if( sCPURayDebug.mCamera.HasValue() )
        cam1 = sCPURayDebug.mCamera.GetValue();
      const Camera* camera{ &cam1 };

      mPixels.resize( mWidth * mHeight );

      const float aspect{ ( float )mWidth / ( float )mHeight };
      const float halfFovYRad{ camera->mFovyrad / 2.0f };

      const float halfNearPlaneHeight { camera->mNearPlane * Tan( halfFovYRad ) };
      const float halfNearPlaneWidth { aspect * halfNearPlaneHeight };
      const v3 nearPlaneX_worldspace{ camera->mRight * halfNearPlaneWidth };
      const v3 nearPlaneY_worldspace{ camera->mUp * halfNearPlaneHeight };
      const v3 nearPlane0_worldspace{ camera->mPos + camera->mForwards * camera->mNearPlane };

      for( int iRow{}; iRow < mHeight; ++iRow )
      {
        for( int iCol{}; iCol < mWidth; ++iCol )
        {
          // directx style uvs
          const float u { ( iCol + 0.5f ) / mWidth };
          const float v { ( iRow + 0.5f ) / mHeight };

          // temp
          if( false )
          {
            const bool isCenterPx{ ( iRow == mHeight / 2 ) && ( iCol == mWidth / 2 ) };
            if( !isCenterPx )
              continue;
          }

          const int iPixel{ iCol + iRow * mWidth };

          const float ndcX{ ( 0 + u ) * 2 - 1 };
          const float ndcY{ ( 1 - v ) * 2 - 1 };

          const v3 pointOnFilm_worldspace{
            nearPlane0_worldspace +
            nearPlaneX_worldspace * ndcX +
            nearPlaneY_worldspace * ndcY };

          v3 radiance{ GetRadiance( camera->mPos, pointOnFilm_worldspace ) };

          mPixels[ iPixel ] = PixelRGBA8Unorm::SetColor3( radiance );
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

  static void RenderSceneCPU( Errors& errors, const Camera* camera, Debug3DDrawData* drawData )
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
    cpuRaytrace.Run( camera, drawData );

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

  void             JPPTPresentation::Render( Render::IContext* ,
                                             const World* world,
                                             const Camera* camera,
                                             const v2i viewSize,
                                             const Render::TextureHandle dstColorTex,
                                             const Render::TextureHandle dstDepthTex,
                                             Errors& errors )
  {
    TAC_UNUSED_PARAMETER( dstDepthTex );
    TAC_UNUSED_PARAMETER( dstColorTex );
    TAC_ASSERT( sInitialized );
    if( !sEnabled )
      return;


    if( sShouldCreateSceneBVH )
    {
      TAC_DELETE sSceneBvh;
      sSceneBvh = SceneBVH::CreateBVH( world, createBVHErrors );
      sShouldCreateSceneBVH = false;
    }

    TAC_CALL( RenderSceneCPU( errors, camera, world->mDebug3DDrawData ) );
    TAC_CALL( RenderSceneGPU( errors ) );

    const float aspect{ ( float )viewSize.x / ( float )viewSize.y};
    sCPURayDebug.DrawFrustum( aspect, world->mDebug3DDrawData );
    if( sCPURayDebug.mCaptureCamera )
    {
      sCPURayDebug.mCaptureCamera = false;
      sCPURayDebug.mCamera = *camera;
    }

    sCPURayDebug.DrawRay(aspect, world->mDebug3DDrawData);
    

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

      sCPURayDebug.DebugImgui();
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

    if( ImGuiCollapsingHeader( "Visualize mesh data", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;

      ImGuiCheckbox( sVisualizeModelSpace ? "(cur: Model Space) " : ( "cur: World Space" ),
                     &sVisualizeModelSpace );

      ImGuiCheckbox( "Visualize Positions", &sVisualizePositions );
      ImGuiDragInt( "Visualize specific position", &sVisualizeSpeficicTriangle );

      if( ImGuiCheckbox( "Visualize Normals", &sVisualizeNormals ) && sVisualizeNormals )
        sVisualizeFrame = false;
      if( sVisualizeNormals || sVisualizeFrame )
        ImGuiDragFloat( "Normal Length", &sVisualizeNormalLength );

      if( ImGuiCheckbox( "Visualize Frame", &sVisualizeFrame ) && sVisualizeFrame )
        sVisualizeNormals = false;

      if( sVisualizeFrame )
        ImGuiDragInt( "Visualize Frame Index", &sVisualizeFrameIndex );
    }

    if( createBVHErrors )
      ImGuiText( createBVHErrors.ToString() );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac

#endif // TAC_JPPT_PRESENTATION_ENABLED
