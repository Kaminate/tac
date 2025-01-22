#include "tac_jppt_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
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
  static Errors               createBVHErrors;
  static bool                 sVisualizePositions;
  static bool                 sVisualizeNormals;
  static float                sVisualizeNormalLength{ 1.0f };
  static bool                 sVisualizeFrame;
  static int                  sVisualizeFrameIndex;
  static Errors               getMeshErrors;

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

    Visualize( world );
    sSceneBvhDebug.DebugVisualizeSceneBVH( world->mDebug3DDrawData, sSceneBvh );
  }

  void             JPPTPresentation::DebugImGui()
  {
    if( !ImGuiCollapsingHeader( "JPPT" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "JPPT Presentation Enabled", &sEnabled );
    if( !sEnabled )
      return;

    if( ImGuiButton( "Create Scene BVH" ) )
    {
      sShouldCreateSceneBVH = true;
    }

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
