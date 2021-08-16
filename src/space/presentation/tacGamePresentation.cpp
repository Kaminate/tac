#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/math/tacMath.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacCamera.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/skybox/tacSkyboxComponent.h"
#include "src/space/model/tacModel.h"
#include "src/space/light/taclight.h"
#include "src/space/physics/tacPhysics.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/space/presentation/tacShadowPresentation.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"
#include "src/space/terrain/tacTerrain.h"

#include <cmath> // std::fmod

namespace Tac
{
#define TAC_PAD_BYTES( byteCount ) char TAC_CONCAT( mPadding, __COUNTER__ )[ byteCount ]



  ShaderFlags shaderLightFlags;
  const auto shaderLightFlagType = shaderLightFlags.Add( 4 );
  const auto shaderLightFlagCastsShadows = shaderLightFlags.Add( 1 );

  // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
  struct ShaderLight
  {
    m4       mWorldToClip;
    v4       mWorldSpacePosition;
    v4       mWorldSpaceUnitDirection;
    v4       mColorRadiance;
    uint32_t mFlags;
    float    mProjA;
    float    mProjB;
    TAC_PAD_BYTES( 4 );
  };


  struct CBufferLights
  {
    static const int TAC_MAX_SHADER_LIGHTS = 4;
    static const int shaderRegister = 2;

    ShaderLight      lights[ TAC_MAX_SHADER_LIGHTS ];
    uint32_t         lightCount;
    uint32_t         useLights;
    uint32_t         testNumber;
  };


  static Render::ShaderHandle          m3DShader;
  static Render::ShaderHandle          mTerrainShader;
  static Render::VertexFormatHandle    m3DVertexFormat;
  static Render::VertexFormatHandle    mTerrainVertexFormat;
  static Render::ConstantBufferHandle  mCBufPerFrame;
  static Render::ConstantBufferHandle  mCBufPerObj;
  static Render::ConstantBufferHandle  mCBufLight;
  static Render::DepthStateHandle      mDepthState;
  static Render::BlendStateHandle      mBlendState;
  static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerStateHandle    mSamplerStateAniso;
  static Render::SamplerStateHandle    mSamplerStatePointShadow;
  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;
  static bool                          mRenderEnabledModel = true;
  static bool                          mRenderEnabledSkybox = true;
  static bool                          mRenderEnabledTerrain = true;
  static bool                          mRenderEnabledDebug3D = true;
  static bool                          mUseLights = true;
  static CBufferLights                 mDebugCBufferLights = {};

  struct TerrainVertex
  {
    v3 mPos;
    v3 mNor;
    v2 mUV;
  };

  struct GameModelVtx
  {
    v3 mPos;
    v3 mNor;
  };

  static void CheckShaderPadding()
  {
    int sizeofshaderlight_estimated = 4 * ( 16 + 4 + 4 + 4 + 1 + 3 );
    int sizeofshaderlight = sizeof( ShaderLight );
    TAC_ASSERT( sizeofshaderlight % 16 == 0 );
    TAC_ASSERT( sizeofshaderlight == sizeofshaderlight_estimated );

    int light1Offset = ( int )TAC_OFFSET_OF( CBufferLights, lights[ 1 ] );
    int light1OffsetReg = light1Offset / 16;
    int light1OffsetAxis = light1Offset % 16;
    TAC_ASSERT( light1OffsetReg == 8 );
    TAC_ASSERT( light1OffsetAxis == 0 );

    int lightCountOffset = ( int )TAC_OFFSET_OF( CBufferLights, lightCount );
    int lightCountOffsetReg = lightCountOffset / 16;
    int lightCountOffsetAxis = lightCountOffset % 16;
    TAC_ASSERT( lightCountOffsetReg == 32 );
    TAC_ASSERT( lightCountOffsetAxis == 0 );
  }

  static DefaultCBufferPerFrame GetPerFrameBuf( const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight )
  {
    const double elapsedSeconds = ShellGetElapsedSeconds();
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                        camera->mNearPlane,
                                        a,
                                        b );

    const float w = ( float )viewWidth;
    const float h = ( float )viewHeight;
    const float aspect = w / h;
    const m4 view = camera->View();
    const m4 proj = camera->Proj( a, b, aspect );
    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = camera->mFarPlane;
    perFrameData.mNear = camera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };
    perFrameData.mSecModTau = ( float )std::fmod( elapsedSeconds, 6.2831853 );
    return perFrameData;
  }

  static TerrainVertex GetTerrainVertex( const Terrain* terrain,
                                         const int r,
                                         const int c,
                                         const v2 uv )
  {
    TerrainVertex terrainVertex = {};
    terrainVertex.mPos = terrain->GetGridVal( r, c );
    terrainVertex.mNor = terrain->GetGridValNormal( r, c );
    terrainVertex.mUV = uv;
    return terrainVertex;
  }

  static void Debug3DEachTri( Graphics* graphics )
  {
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             GamePresentationGetVertexDeclarations(),
                                                             errors );
        if( !mesh )
          return;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          for( const SubMeshTriangle& tri : subMesh.mTris )
          {
            const v3 p0 = ( model->mEntity->mWorldTransform *  v4( tri[ 0 ], 1 ) ).xyz();
            const v3 p1 = ( model->mEntity->mWorldTransform *  v4( tri[ 1 ], 1 ) ).xyz();
            const v3 p2 = ( model->mEntity->mWorldTransform *  v4( tri[ 2 ], 1 ) ).xyz();
            mDrawData->DebugDraw3DTriangle( p0, p1, p2 );
          }
        }
      }
      Debug3DDrawData* mDrawData;
    } visitor = {};
    visitor.mDrawData = graphics->mWorld->mDebug3DDrawData;

    graphics->VisitModels( &visitor );
  }

  static Mesh* LoadModel( const Model* model )
  {
    Errors getmeshErrors;
    return ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                   model->mModelIndex,
                                                   m3DVertexFormatDecls,
                                                   getmeshErrors );
  }

  static void RenderGameWorldAddDrawCall( const Light** lights,
                                          int lightCount,
                                          const Model* model,
                                          const Render::ViewHandle viewId )
  {
    const Mesh* mesh = LoadModel( model );
    if( !mesh )
      return;

    DefaultCBufferPerObject perObjectData;
    perObjectData.Color = { model->mColorRGB, 1 };
    perObjectData.World = model->mEntity->mWorldTransform;

    Render::DrawCallTextures drawCallTextures;
    if( mUseLights )
    {
      CBufferLights cBufferLights = {};
      cBufferLights.lightCount = lightCount;
      cBufferLights.useLights = true;
      cBufferLights.testNumber = 1234567890;
      for( int i = 0; i < lightCount; ++i )
      {
        drawCallTextures.push_back( lights[ i ]->mShadowMapDepth );
        const Light* light = lights[ i ];
        Camera camera = light->GetCamera();
        float a;
        float b;
        Render::GetPerspectiveProjectionAB( camera.mFarPlane,
                                            camera.mNearPlane,
                                            a,
                                            b );
        const float w = ( float )light->mShadowResolution;
        const float h = ( float )light->mShadowResolution;
        const float aspect = w / h;
        const m4 view = camera.View();
        const m4 proj = camera.Proj( a, b, aspect );


        const uint32_t flags = 0
          | shaderLightFlagType.ShiftResult( light->mType )
          | shaderLightFlagCastsShadows.ShiftResult( light->mCastsShadows );

        ShaderLight* shaderLight = &cBufferLights.lights[ i ];
        shaderLight->mColorRadiance.xyz() = light->mColor;
        shaderLight->mColorRadiance.w = light->mRadiance;
        shaderLight->mFlags = flags;
        shaderLight->mWorldSpaceUnitDirection.xyz() = light->GetUnitDirection();
        shaderLight->mWorldSpacePosition.xyz() = light->mEntity->mWorldPosition;
        shaderLight->mWorldToClip = proj * view;
        //shaderLight->mNear = camera.mNearPlane;
        //shaderLight->mFar = camera.mFarPlane;
        shaderLight->mProjA = a;
        shaderLight->mProjB = b;

        //v4 r0 = shaderLight->mWorldToClip.GetRow( 0 );
        //v4 r1 = shaderLight->mWorldToClip.GetRow( 1 );
        //v4 r2 = shaderLight->mWorldToClip.GetRow( 2 );
        //v4 r3 = shaderLight->mWorldToClip.GetRow( 3 );
        //const char* debugMtx = FrameMemoryPrintf(
        //  "%.2f %.2f %.2f %.2f\n"
        //  "%.2f %.2f %.2f %.2f\n"
        //  "%.2f %.2f %.2f %.2f\n"
        //  "%.2f %.2f %.2f %.2f\n",
        //  r0.x, r0.y, r0.z, r0.w,
        //  r1.x, r1.y, r1.z, r1.w,
        //  r2.x, r2.y, r2.z, r2.w,
        //  r3.x, r3.y, r3.z, r3.w);

        //v4 worldspacePos = {0,0,0,1};
        //v4 clipspacePos = shaderLight->mWorldToClip * worldspacePos;
        //v4 ndcspacePos = clipspacePos / clipspacePos.w;

        //float clipToViewPos = ( clipspacePos.z - b ) / a;
        //float ndcToViewPos = -b / ( a + ndcspacePos.z );



      }
      Render::UpdateConstantBuffer( mCBufLight, &cBufferLights, sizeof( CBufferLights ), TAC_STACK_FRAME );

      mDebugCBufferLights = cBufferLights;
    }

    Render::SetTexture( drawCallTextures );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const  char* groupName = FrameMemoryPrintf( "%s %s", model->mEntity->mName.c_str(), subMesh.mName.c_str() );
      Render::BeginGroup( groupName, TAC_STACK_FRAME );
      //FrameMemoryPrintf( "%s %i", subMesh.mName.c_str(),
      //                                         model->mModelPath.c_str(),
      //                                         model->mModelIndex ), TAC_STACK_FRAME );
      Render::SetShader( m3DShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( { mSamplerStateAniso, mSamplerStatePointShadow } );
      Render::SetDepthState( mDepthState );
      Render::SetVertexFormat( m3DVertexFormat );
      Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      Render::UpdateConstantBuffer( mCBufPerObj,
                                    &perObjectData,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::Submit( viewId, TAC_STACK_FRAME );
      Render::EndGroup( TAC_STACK_FRAME );
    }
  }

  static void LoadTerrain( Terrain* terrain )
  {
    if( terrain->mVertexBuffer.IsValid() && terrain->mIndexBuffer.IsValid() )
      return;

    if( terrain->mRowMajorGrid.empty() )
      return;

    //OSDebugBreak();

    typedef uint32_t TerrainIndex;


    Vector< TerrainVertex > vertexes;
    Vector< TerrainIndex > indexes;

    for( int iRow = 1; iRow < terrain->mSideVertexCount; ++iRow )
    {
      for( int iCol = 1; iCol < terrain->mSideVertexCount; ++iCol )
      {
        const int iVertexTL = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol - 1, { 0, 1 } ) );

        const int iVertexTR = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol, { 1, 1 } ) );

        const int iVertexBL = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow, iCol - 1, { 0, 0 } ) );

        const int iVertexBR = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow, iCol, { 1, 0 } ) );

        // lower left triangle
        indexes.push_back( iVertexBR );
        indexes.push_back( iVertexTL );
        indexes.push_back( iVertexBL );

        // upper right triangle
        indexes.push_back( iVertexBR );
        indexes.push_back( iVertexTR );
        indexes.push_back( iVertexTL );
      }
    }

    terrain->mVertexBuffer = Render::CreateVertexBuffer( vertexes.size() * sizeof( TerrainVertex ),
                                                         vertexes.data(),
                                                         sizeof( TerrainVertex ),
                                                         Render::Access::Default,
                                                         TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( terrain->mVertexBuffer, "terrain-vtx-buf" );

    Render::Format format;
    format.mElementCount = 1;
    format.mPerElementByteCount = sizeof( TerrainIndex );
    format.mPerElementDataType = Render::GraphicsType::uint;
    terrain->mIndexBuffer = Render::CreateIndexBuffer( indexes.size() * sizeof( TerrainIndex ),
                                                       indexes.data(),
                                                       Render::Access::Default,
                                                       format,
                                                       TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( terrain->mIndexBuffer, "terrain-idx-buf" );
    terrain->mIndexCount = indexes.size();

  }

  static void CreateTerrainShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mTerrainShader = Render::CreateShader( Render::ShaderSource::FromPath( "Terrain" ),
                                           Render::ConstantBuffers{ mCBufPerFrame, mCBufPerObj },
                                           TAC_STACK_FRAME );
  }

  static void Create3DShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    m3DShader = Render::CreateShader( Render::ShaderSource::FromPath( "GamePresentation" ),
                                      Render::ConstantBuffers{ mCBufPerFrame, mCBufPerObj },
                                      TAC_STACK_FRAME );
  }

  static void Create3DVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );

    const Render::VertexDeclaration posDecl = []()
    {
      Render::VertexDeclaration vertexDeclaration = {};
      vertexDeclaration.mAttribute = Render::Attribute::Position;
      vertexDeclaration.mTextureFormat.mElementCount = 3;
      vertexDeclaration.mTextureFormat.mPerElementByteCount = sizeof( float );
      vertexDeclaration.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
      vertexDeclaration.mAlignedByteOffset = TAC_OFFSET_OF( GameModelVtx, mPos );
      return vertexDeclaration;
    }( );

    const Render::VertexDeclaration norDecl = []()
    {
      Render::VertexDeclaration vertexDeclaration = {};
      vertexDeclaration.mAttribute = Render::Attribute::Normal;
      vertexDeclaration.mTextureFormat.mElementCount = 3;
      vertexDeclaration.mTextureFormat.mPerElementByteCount = sizeof( float );
      vertexDeclaration.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
      vertexDeclaration.mAlignedByteOffset = TAC_OFFSET_OF( GameModelVtx, mNor );
      return vertexDeclaration;
    }( );

    m3DVertexFormatDecls = Render::VertexDeclarations{ posDecl, norDecl };
    m3DVertexFormat = Render::CreateVertexFormat( m3DVertexFormatDecls,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m3DVertexFormat, "game-3d-vtx-fmt" );
  }

  static void CreateTerrainVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::VertexDeclaration terrainPosDecl = {};
    terrainPosDecl.mAttribute = Render::Attribute::Position;
    terrainPosDecl.mTextureFormat.mElementCount = 3;
    terrainPosDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainPosDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainPosDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mPos );

    Render::VertexDeclaration terrainNorDecl = {};
    terrainNorDecl.mAttribute = Render::Attribute::Normal;
    terrainNorDecl.mTextureFormat.mElementCount = 3;
    terrainNorDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainNorDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainNorDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mNor );

    Render::VertexDeclaration terrainTexCoordDecl = {};
    terrainTexCoordDecl.mAttribute = Render::Attribute::Texcoord;
    terrainTexCoordDecl.mTextureFormat.mElementCount = 2;
    terrainTexCoordDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainTexCoordDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainTexCoordDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mUV );

    mTerrainVertexFormat = Render::CreateVertexFormat( { terrainPosDecl, terrainNorDecl, terrainTexCoordDecl },
                                                       mTerrainShader,
                                                       TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mTerrainVertexFormat, "terrain-vtx-fmt" );
  }

  static void CreatePerFrame( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mCBufPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                                  DefaultCBufferPerFrame::shaderRegister,
                                                  TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mCBufPerFrame, "game-per-frame" );
  }


  static void CreateCBufLights()
  {
    mCBufLight = Render::CreateConstantBuffer( sizeof( CBufferLights ),
                                               CBufferLights::shaderRegister,
                                               TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mCBufLight, "game-lights" );
  }

  static void CreatePerObj( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mCBufPerObj = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerObject ),
                                                DefaultCBufferPerObject::shaderRegister,
                                                TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mCBufPerObj, "game-per-obj" );
  }

  static void CreateDepthState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = Render::DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
  }

  static void CreateBlendState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = Render::BlendConstants::One;
    blendStateData.mDstRGB = Render::BlendConstants::Zero;
    blendStateData.mBlendRGB = Render::BlendMode::Add;
    blendStateData.mSrcA = Render::BlendConstants::Zero;
    blendStateData.mDstA = Render::BlendConstants::One;
    blendStateData.mBlendA = Render::BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
  }

  static void CreateRasterizerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None;
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
  }

  static void CreateSamplerStateShadow( Errors& errors )
  {

    TAC_UNUSED_PARAMETER( errors );
    Render::SamplerState data;
    data.mFilter = Render::Filter::Point;
    mSamplerStatePointShadow = Render::CreateSamplerState( data, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerStateAniso, "game-shadow-samp" );
  }

  static void CreateSamplerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Render::Filter::Aniso;
    mSamplerStateAniso = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerStateAniso, "game-samp" );
  }

  static void RenderModels( World* world,
                            const Camera* camera,
                            const int viewWidth,
                            const int viewHeight,
                            const Render::ViewHandle viewId )
  {
    if( !mRenderEnabledModel )
      return;
    const DefaultCBufferPerFrame perFrameData = GetPerFrameBuf( camera,
                                                                viewWidth,
                                                                viewHeight );
    Render::UpdateConstantBuffer( mCBufPerFrame,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    Graphics* graphics = GetGraphics( world );
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        static struct : public LightVisitor
        {
          void operator()( Light* light ) override { mLights.push_back( light ); }
          Vector< const Light* > mLights;
        } sLightVisitor;

        sLightVisitor.mLights.resize( 0 );

        if( mUseLights )
          mGraphics->VisitLights( &sLightVisitor );

        sLightVisitor.mLights.resize( Min( sLightVisitor.mLights.size(), CBufferLights::TAC_MAX_SHADER_LIGHTS ) );


        RenderGameWorldAddDrawCall( sLightVisitor.mLights.data(),
                                    sLightVisitor.mLights.size(),
                                    model,
                                    mViewId );
      }

      Render::ViewHandle mViewId;
      //World*             mWorld;
      Graphics*          mGraphics;
    } myModelVisitor;
    myModelVisitor.mViewId = viewId;
    myModelVisitor.mGraphics = graphics;
    //myModelVisitor.mWorld = world;

    if( !mUseLights )
    {
      CBufferLights cBufferLights = {};
      Render::UpdateConstantBuffer( mCBufLight, &cBufferLights, sizeof( CBufferLights ), TAC_STACK_FRAME );
    }


    TAC_RENDER_GROUP_BLOCK( "Visit Models" );
    //Render::BeginGroup( "Visit Models", TAC_STACK_FRAME );
    graphics->VisitModels( &myModelVisitor );
    //Render::EndGroup( TAC_STACK_FRAME );
  }

  static void RenderTerrain( World* world,
                             const Camera* camera,
                             const int viewWidth,
                             const int viewHeight,
                             const Render::ViewHandle viewId )
  {
    if( !mRenderEnabledTerrain )
      return;
    const DefaultCBufferPerFrame perFrameData = GetPerFrameBuf( camera,
                                                                viewWidth,
                                                                viewHeight );
    Render::UpdateConstantBuffer( mCBufPerFrame,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );
    Physics* physics = Physics::GetSystem( world );

    TAC_RENDER_GROUP_BLOCK( "Visit Terrains" );
    //Render::BeginGroup( "Visit Terrains", TAC_STACK_FRAME );
    for( Terrain* terrain : physics->mTerrains )
    {
      LoadTerrain( terrain );

      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
        continue;

      const Render::TextureHandle terrainTexture =
        TextureAssetManager::GetTexture( terrain->mGroundTexturePath, mGetTextureErrorsGround );
      const Render::TextureHandle noiseTexture =
        TextureAssetManager::GetTexture( terrain->mNoiseTexturePath, mGetTextureErrorsNoise );

      DefaultCBufferPerObject cbuf = {};
      cbuf.Color = { 1, 1, 1, 1 };
      cbuf.World = m4::Identity();

      Render::SetTexture( Render::DrawCallTextures{ terrainTexture, noiseTexture } );
      Render::UpdateConstantBuffer( mCBufPerObj, &cbuf, sizeof( DefaultCBufferPerObject ), TAC_STACK_FRAME );
      Render::SetDepthState( mDepthState );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerStateAniso );
      Render::SetShader( mTerrainShader );
      Render::SetIndexBuffer( terrain->mIndexBuffer, 0, terrain->mIndexCount );
      Render::SetVertexBuffer( terrain->mVertexBuffer, 0, 0 );
      Render::SetVertexFormat( mTerrainVertexFormat );
      Render::Submit( viewId, TAC_STACK_FRAME );
    }
    //Render::EndGroup( TAC_STACK_FRAME );
  }


  static void RenderSkybox( World* world,
                            const Camera* camera,
                            const int viewWidth,
                            const int viewHeight,
                            const Render::ViewHandle viewId )
  {

    if( !mRenderEnabledSkybox )
      return;
    Graphics* graphics = GetGraphics( world );
    struct : public SkyboxVisitor
    {
      void operator()( Skybox* skybox ) override
      {
        SkyboxPresentationRender( mCamera,
                                  mViewWidth,
                                  mViewHeight,
                                  mViewId,
                                  skybox->mSkyboxDir );
      }
      int                mViewWidth;
      int                mViewHeight;
      Render::ViewHandle mViewId;
      const Camera*      mCamera;
    } mySkyboxVisitor;
    mySkyboxVisitor.mViewWidth = viewWidth;
    mySkyboxVisitor.mViewHeight = viewHeight;
    mySkyboxVisitor.mViewId = viewId;
    mySkyboxVisitor.mCamera = camera;
    TAC_RENDER_GROUP_BLOCK( "Visit Skyboxes" );
    graphics->VisitSkyboxes( &mySkyboxVisitor );
  }

  const Mesh* GamePresentationGetModelMesh( const Model* model )
  {
    return LoadModel( model );
  }

  void        GamePresentationInit( Errors& errors )
  {
    CheckShaderPadding();

    CreatePerFrame( errors );
    TAC_HANDLE_ERROR( errors );

    CreateCBufLights();

    CreatePerObj( errors );
    TAC_HANDLE_ERROR( errors );

    Create3DShader( errors );
    TAC_HANDLE_ERROR( errors );

    CreateTerrainShader( errors );
    TAC_HANDLE_ERROR( errors );

    Create3DVertexFormat( errors );
    TAC_HANDLE_ERROR( errors );

    CreateTerrainVertexFormat( errors );
    TAC_HANDLE_ERROR( errors );

    CreateBlendState( errors );
    TAC_HANDLE_ERROR( errors );

    CreateDepthState( errors );
    TAC_HANDLE_ERROR( errors );

    CreateRasterizerState( errors );
    TAC_HANDLE_ERROR( errors );

    CreateSamplerState( errors );
    TAC_HANDLE_ERROR( errors );

    CreateSamplerStateShadow( errors );
  }

  void        GamePresentationUninit()
  {
    Render::DestroyShader( m3DShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( m3DVertexFormat, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mCBufPerFrame, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mCBufLight, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mCBufPerObj, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerStateAniso, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerStatePointShadow, TAC_STACK_FRAME );
  }


  void        GamePresentationRender( World* world,
                                      const Camera* camera,
                                      const int viewWidth,
                                      const int viewHeight,
                                      const Render::ViewHandle viewId )
  {
    TAC_RENDER_GROUP_BLOCK( "GamePresentationRender" );

    ShadowPresentationRender( world );

    RenderModels( world, camera, viewWidth, viewHeight, viewId );

    RenderTerrain( world, camera, viewWidth, viewHeight, viewId );

    // Skybox should be last to reduce pixel shader invocations
    RenderSkybox( world,
                  camera,
                  viewWidth,
                  viewHeight,
                  viewId );

    if( mRenderEnabledDebug3D )
    {
      Errors e;
      world->mDebug3DDrawData->DebugDraw3DToTexture( viewId, camera, viewWidth, viewHeight, e );
    }
  }

  Render::ConstantBufferHandle  GamePresentationGetPerFrame()             { return mCBufPerFrame; }
  Render::ConstantBufferHandle  GamePresentationGetPerObj()               { return mCBufPerObj; }
  Render::DepthStateHandle      GamePresentationGetDepthState()           { return mDepthState; }
  Render::BlendStateHandle      GamePresentationGetBlendState()           { return mBlendState; }
  Render::RasterizerStateHandle GamePresentationGetRasterizerState()      { return mRasterizerState; }
  Render::SamplerStateHandle    GamePresentationGetSamplerState()         { return mSamplerStateAniso; }
  Render::VertexDeclarations    GamePresentationGetVertexDeclarations()   { return m3DVertexFormatDecls; }
  Render::VertexFormatHandle    GamePresentationGetVertexFormat()         { return m3DVertexFormat; }


  void                          GamePresentationDebugImGui( Graphics* graphics )
  {
    if( !ImGuiCollapsingHeader( "Game Presentation" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "Game Presentation Enabled Model", &mRenderEnabledModel );
    ImGuiCheckbox( "Game Presentation Enabled Skybox", &mRenderEnabledSkybox );
    ImGuiCheckbox( "Game Presentation Enabled Terrain", &mRenderEnabledTerrain );
    ImGuiCheckbox( "Game Presentation Enabled Debug3D", &mRenderEnabledDebug3D );

    ImGuiCheckbox( "Game Presentation use lights", &mUseLights );
    if( mUseLights && ImGuiCollapsingHeader( "CBufferLights" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiText( FrameMemoryPrintf( "light count %i", mDebugCBufferLights.lightCount ) );
      ImGuiText( FrameMemoryPrintf( "text number %i", mDebugCBufferLights.testNumber ) );
      for( int iLight = 0; iLight < ( int )mDebugCBufferLights.lightCount; ++iLight )
      {
        if( !ImGuiCollapsingHeader( FrameMemoryPrintf( "Light %i", iLight ) ) )
          continue;

        ShaderLight* shaderLight = &mDebugCBufferLights.lights[ iLight ];
        String rowsStrs[ 4 ];
        for( int r = 0; r < 4; ++r )
          for( int c = 0; c < 4; ++c )
            rowsStrs[ r ] += FrameMemoryPrintf( "%.2f ", shaderLight->mWorldToClip( r, c ) );

        const Light::Type lightType = ( Light::Type )shaderLightFlagType.Extract( shaderLight->mFlags );
        const bool castsShadows = ( bool )shaderLightFlagCastsShadows.Extract( shaderLight->mFlags );

        TAC_IMGUI_INDENT_BLOCK;
        ImGuiText( "World to clip matrix" );
        ImGuiText( rowsStrs[ 0 ] );
        ImGuiText( rowsStrs[ 1 ] );
        ImGuiText( rowsStrs[ 2 ] );
        ImGuiText( rowsStrs[ 3 ] );
        ImGuiDragFloat4( "worldspace pos", shaderLight->mWorldSpacePosition.data() );
        ImGuiDragFloat3( "worldspace pos", shaderLight->mWorldSpacePosition.data() );
        ImGuiDragFloat3( "worldspace unit dir", shaderLight->mWorldSpaceUnitDirection.data() );
        ImGuiDragFloat3( "light color", shaderLight->mColorRadiance.data() );
        ImGuiDragFloat( "light radiance", &shaderLight->mColorRadiance.w );
        //ImGuiDragFloat( "light near plane", &shaderLight->mNear );
        //ImGuiDragFloat( "light far plane", &shaderLight->mFar );
        ImGuiDragFloat( "light proj a", &shaderLight->mProjA );
        ImGuiDragFloat( "light proj b", &shaderLight->mProjB );
        ImGuiImage( -1, v2( 1, 1 ) * 50, v4( shaderLight->mColorRadiance.xyz(), 1.0f ) );
        ImGuiText( FrameMemoryPrintf( "Light type: %s (%i)",
                                      LightTypeToString( lightType ),
                                      ( int )lightType ) );
        ImGuiText( FrameMemoryPrintf( "casts shadows: %s", ( castsShadows ? "true" : "false" ) ) );
      }
    }

    if( mRenderEnabledDebug3D )
    {
      static bool debugEachTri;
      ImGuiCheckbox( "debug each tri", &debugEachTri );
      if( debugEachTri )
      {
        Debug3DEachTri( graphics );
      }
    }
  }
}
