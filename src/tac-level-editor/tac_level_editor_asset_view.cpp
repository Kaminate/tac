#include "tac_level_editor_asset_view.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_resident_model_file.h"
#include "tac-engine-core/assetmanagers/obj/tac_model_asset_loader_obj.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_game_timer.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{

  struct AssetViewImportedModel
  {
    World                     mWorld                 {};
    Camera                    mCamera                {};
    EntityUUIDCounter         mEntityUUIDCounter     {};
    bool                      mAttemptedToLoadEntity {};
    Entity*                   mPrefab                {};
    Render::TextureHandle     mTextureHandleColor    {};
    Render::TextureHandle     mTextureHandleDepth    {};
    AssetPathString           mAssetPath             {};
    Debug3DDrawBuffers        mDebug3DDrawBuffers    {};
  };

  // -----------------------------------------------------------------------------------------------

  using LoadedModels = Vector< AssetViewImportedModel* >;

  static AssetPathString           sAssetViewFolderCur;
  static Vector< String >          sAssetViewFolderStack; // ie { "assets", "editor" }
  static Errors                    sAssetViewErrors;
  static AssetPathStrings          sAssetViewFiles;
  static AssetPathStrings          sAssetViewFolders;
  static LoadedModels              sAssetViewLoadedModels;
  static const int                 sAssetViewWidth { 256 };
  static const int                 sAssetViewHeight { 256 };

  // -----------------------------------------------------------------------------------------------


  static auto DecomposeGLTFTransform( const cgltf_node* node ) -> RelativeSpace
  {
    cgltf_float local[ 16 ]{};
    cgltf_node_transform_local( node, local );

    m4 mLocal {};
    for( int i {}; i < 16; ++i )
      mLocal[ i ] = local[ i ];
    mLocal.Transpose();

    v3 c0 { mLocal.m00, mLocal.m10, mLocal.m20 };
    v3 c1 { mLocal.m01, mLocal.m11, mLocal.m21 };
    v3 c2 { mLocal.m02, mLocal.m12, mLocal.m22 };
    v3 c3 { mLocal.m03, mLocal.m13, mLocal.m23 };
    const v3 scale { Length( c0 ), Length( c1 ), Length( c2 ) };
    c0 /= scale.x;
    c1 /= scale.y;
    c2 /= scale.z;

    float
      r11, r12, r13,
      r21, r22, r23,
      r31, r32, r33;
    {
      r11 = c0[ 0 ];
      r21 = c0[ 1 ];
      r31 = c0[ 2 ];

      r12 = c1[ 0 ];
      r22 = c1[ 1 ];
      r32 = c1[ 2 ];

      r13 = c2[ 0 ];
      r23 = c2[ 1 ];
      r33 = c2[ 2 ];
    }

    // On-rotation-deformation-zones-for-finite-strain-Cosserat-plasticity.pdf
    // Rot( x, y, z ) = rotZ(phi) * rotY(theta) * rotX(psi)
    float zPhi{};
    float yTheta{};
    float xPsi{};


    if( r31 != 1.0f && r31 != -1.0f )
    {
      yTheta = -Asin( r31 );
      xPsi = Atan2( r32 / Cos( yTheta ), r33 / Cos( yTheta ) );
      zPhi = Atan2( r21 / Cos( yTheta ), r11 / Cos( yTheta ) );
    }
    else
    {
      zPhi = 0;
      if( r31 == -1.0f )
      {
        yTheta = 3.14f / 2.0f;
        xPsi = Atan2( r12, r13 );
      }
      else
      {
        yTheta = -3.14f / 2.0f;
        xPsi = Atan2( -r12, -r13 );
      }
    }

    const v3 eulerRads { xPsi, yTheta, zPhi };
    return RelativeSpace
    {
      .mPosition  { c3 },
      .mEulerRads { eulerRads },
      .mScale     { scale },
    };
  }

  static void AssetImporterAttemptLoadEntity( AssetViewImportedModel* loadedModel,
                                   const AssetPathStringView& path,
                                   Errors& errors )
  {
    if( loadedModel->mAttemptedToLoadEntity )
      return;

#if 0
    if( path.ends_with( ".obj" ) )
    {
      TAC_ON_DESTRUCT( loadedModel->mAttemptedToLoadEntity = true );

      ++asdf;
      // todo: async
      TAC_CALL( const String fileStr{ LoadFilePath( path, errors ) } );
      const void* fileBytes{ fileStr.data() };
      const int fileByteCount{ fileStr.size() };
      const WavefrontObj wavefrontObj{ WavefrontObj::Load( fileBytes, fileByteCount ) };
    }
    else
#endif
    {
      const cgltf_data* gltfData{ TryGetGLTFData( path ) };
      if( !gltfData )
        return;

      TAC_ON_DESTRUCT( loadedModel->mAttemptedToLoadEntity = true );
      loadedModel->mPrefab = loadedModel->mWorld.SpawnEntity(
        loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
      loadedModel->mPrefab->mName = path.GetFilename();
      const int nNodes{ ( int )gltfData->nodes_count };
      Vector< Entity* > entityNodes( nNodes );
      for( int i{}; i < nNodes; ++i )
        entityNodes[ i ] = loadedModel->mWorld.SpawnEntity(
          loadedModel->mEntityUUIDCounter.AllocateNewUUID() );

      for( int i{}; i < nNodes; ++i )
      {
        const cgltf_node& gltfNode{ gltfData->nodes[ i ] };
        const cgltf_mesh* gltfMesh{ gltfNode.mesh };
        const String name
        {
          [ & ]() -> String
          {
            if( gltfNode.name )
              return gltfNode.name;

            if( gltfMesh && gltfMesh->name )
                return gltfMesh->name;

            return String() + "node " + ToString( i );
          }()
        };

        Entity* entity{ entityNodes[ i ] };
        entity->mName = name;
        entity->mRelativeSpace = DecomposeGLTFTransform( &gltfNode );
        if( gltfMesh )
        {
          Model* model{ ( Model* )entity->AddNewComponent( Model().GetEntry() ) };
          model->mModelPath = path;
          model->mModelIndex = ( int )( gltfMesh - gltfData->meshes );

          TAC_ASSERT( gltfMesh->primitives_count == 1 );
          const cgltf_primitive* gltfPrimitive{ &gltfMesh->primitives[ 0 ] };
          const cgltf_material* gltfMaterial{ gltfPrimitive->material };
          if( gltfMaterial )
          {
            if( gltfMaterial->has_pbr_metallic_roughness )
            {
              const cgltf_pbr_metallic_roughness* pbr_metallic_roughness{
                &gltfMaterial->pbr_metallic_roughness };
              const v4 color( pbr_metallic_roughness->base_color_factor[ 0 ],
                              pbr_metallic_roughness->base_color_factor[ 1 ],
                              pbr_metallic_roughness->base_color_factor[ 2 ],
                              pbr_metallic_roughness->base_color_factor[ 3 ] );

              const v3 emissive = v3( gltfMaterial->emissive_factor[ 0 ],
                                      gltfMaterial->emissive_factor[ 1 ],
                                      gltfMaterial->emissive_factor[ 2 ] ) *
                ( gltfMaterial->has_emissive_strength
                  ? gltfMaterial->emissive_strength.emissive_strength
                  : 1 );

              Material* ecsMaterial{ ( Material* )entity->AddNewComponent( Material{}.GetEntry() ) };
              ecsMaterial->mShaderGraph = "assets/shader-graphs/gltf_pbr.tac.sg";
              ecsMaterial->mIsGlTF_PBR_MetallicRoughness = true;
              ecsMaterial->mPBR_Factor_Metallic = pbr_metallic_roughness->metallic_factor;
              ecsMaterial->mPBR_Factor_Roughness = pbr_metallic_roughness->roughness_factor;
              ecsMaterial->mColor = color;
              ecsMaterial->mEmissive = emissive;

              if( pbr_metallic_roughness->base_color_texture.texture )
              {
                TAC_ASSERT_UNIMPLEMENTED;
              }

              if( pbr_metallic_roughness->metallic_roughness_texture.texture )
              {
                TAC_ASSERT_UNIMPLEMENTED;
              }

            }
            else
            {
              TAC_ASSERT_UNIMPLEMENTED;
            }
          }
        }

        const int nChildren{ ( int )gltfNode.children_count };
        for( int iChild{}; iChild < nChildren; ++iChild )
        {
          const cgltf_node* gltfChild{ gltfNode.children[ iChild ] };
          Entity* ecsChild{ entityNodes[ gltfChild - gltfData->nodes ] };
          // shouldnt this be recursive
          entity->AddChild( ecsChild );
        }

        if( !gltfNode.parent )
          loadedModel->mPrefab->AddChild( entity );
      }
    }

    loadedModel->mWorld.ComputeTransformsRecursively();
  }


  static auto LoadEllipses() { return String( "...", ( int )GameTimer::GetElapsedTime() % 4 ); }

  static auto HasExt( const AssetPathStringView path, Vector< const char* > extensions )
  {
    for( const char* ext : extensions )
      if( path.GetFileExtension() == ( StringView )ext )
        return true;
    return false;
  }

  static bool IsImage( const AssetPathStringView path )
  {
    Vector< const char* > exts;
    exts.push_back( ".png" );
    exts.push_back( ".jpg" );
    exts.push_back( ".bmp" );
    return HasExt( path, exts );
  }

  static bool IsModel( const AssetPathStringView path )
  {
    Vector< const char* > exts;
    exts.push_back( ".gltf" );
    exts.push_back( ".glb" );
    //exts.push_back( ".obj" );
    return HasExt( path, exts );
  }

  static bool IsOther( const AssetPathStringView path )
  {
    return !IsImage(path) && !IsModel(path);
  }

  static auto GetPathsUsingFn( bool ( *fn )( AssetPathStringView ) ) -> AssetPathStrings
  {
    AssetPathStrings paths;
    for( AssetPathStringView path : sAssetViewFiles )
      if( fn( path ) )
        paths.push_back( path );
    return paths;
  }

  static auto GetImagePaths() { return GetPathsUsingFn( IsImage ); }
  static auto GetModelPaths() { return GetPathsUsingFn( IsModel ); }
  static auto GetOtherPaths() { return GetPathsUsingFn( IsOther ); }

  static auto GetLoadedModel( const AssetPathStringView& path ) -> AssetViewImportedModel*
  {
    for( AssetViewImportedModel* model : sAssetViewLoadedModels )
      if( ( StringView )model->mAssetPath == path )
        return model;

    return nullptr;
  }

  static void PopulateFolderContents( Errors& errors )
  {
    TAC_CALL( sAssetViewFiles = IterateAssetsInDir( sAssetViewFolderCur,
                                                       AssetIterateType::Default,
                                                       errors ) );
    TAC_CALL( sAssetViewFolders = IterateAssetsDirs( sAssetViewFolderCur,
                                                               AssetIterateType::Default,
                                                               errors ) );
  }

  static auto CreateLoadedModel( const AssetPathStringView& assetPath,
                                 Errors& errors ) -> AssetViewImportedModel*
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const String colorParamsName{ "AssetView " + assetPath + " color" };
    const Render::CreateTextureParams texSpecColor
    {
      .mImage        { Render::Image {
        .mWidth      { sAssetViewWidth },
        .mHeight     { sAssetViewHeight },
        .mFormat     { Render::TexFmt::kRGBA16F }, } },
      .mMipCount     { 1 },
      .mBinding      { Render::Binding::ShaderResource | Render::Binding::RenderTarget },
      .mOptionalName { colorParamsName },
    };

    TAC_CALL_RET( const Render::TextureHandle textureHandleColor{
      renderDevice->CreateTexture( texSpecColor, errors ) } );

    const String depthParamsName{ "AssetView " + assetPath + " depth" };
    const Render::CreateTextureParams texSpecDepth
    {
      .mImage        { Render::Image{
        .mWidth      { sAssetViewWidth },
        .mHeight     { sAssetViewHeight },
        .mFormat     { Render::TexFmt::kD24S8 } } },
      .mMipCount     { 1 },
      .mBinding      { Render::Binding::DepthStencil },
      .mOptionalName { depthParamsName },
    };

    TAC_CALL_RET( const Render::TextureHandle textureHandleDepth{
      renderDevice->CreateTexture( texSpecDepth, errors ) } );

    auto result{ TAC_NEW AssetViewImportedModel
    {
       .mTextureHandleColor { textureHandleColor },
       .mTextureHandleDepth { textureHandleDepth },
       .mAssetPath          { assetPath },
    } };

    sAssetViewLoadedModels.push_back( result );
    return result;
  }

  static void RenderImportedModel( Render::IContext* renderContext,
                                   AssetViewImportedModel* loadedModel,
                                   Errors& errors )
  {
    if( loadedModel->mWorld.mEntities.empty() )
      return;

    const v2i viewSize{ sAssetViewWidth, sAssetViewHeight };
    const ShortFixedString groupName{ ShortFixedString::Concat( "asset preview ",
                            loadedModel->mAssetPath.c_str() ) };

    TAC_RENDER_GROUP_BLOCK( renderContext, groupName );

    const Render::Targets renderTargets
    {
      .mColors { loadedModel->mTextureHandleColor },
      .mDepth  { loadedModel->mTextureHandleDepth },
    };
    renderContext->ClearColor(loadedModel->mTextureHandleColor, v4( 0, 0, 0, 1 ) );
    renderContext->ClearDepth(loadedModel->mTextureHandleDepth, 1.0f);
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );

    const GamePresentation::RenderParams renderParams
    {
      .mContext  { renderContext },
      .mWorld    { &loadedModel->mWorld },
      .mCamera   { &loadedModel->mCamera },
      .mViewSize { viewSize },
      .mColor    { loadedModel->mTextureHandleColor },
      .mDepth    { loadedModel->mTextureHandleDepth },
      .mBuffers  { &loadedModel->mDebug3DDrawBuffers },
    };
    GamePresentation::Render( renderParams, errors );
  }

  static void UIFoldersUpToCurr()
  {
    TAC_ASSERT( !sAssetViewFolderStack.empty() );

    ImGuiText( "Path: " );
    ImGuiSameLine();

    for( int iStack {}; iStack < sAssetViewFolderStack.size(); ++iStack )
    {
      const UTF8Path& folder { sAssetViewFolderStack[ iStack ] };
      if( ImGuiButton( folder ) )
      {
        sAssetViewFolderStack.resize( iStack + 1 );
      }

      if( iStack != sAssetViewFolderStack.size() - 1 )
      {
        ImGuiSameLine();
        ImGuiText( "/" );
        ImGuiSameLine();
      }
    }
  }

  static void UIFoldersNext( Errors& errors )
  {
    const int n { sAssetViewFolders.size() };
    for( AssetPathStringView asset : sAssetViewFolders )
    {
      if( const String folder{ asset.GetFilename() };
          ImGuiButton( folder ) )
        sAssetViewFolderStack.push_back( folder );
    }
  }

  static void UIFilesOther( Errors& errors )
  {
    const AssetPathStrings paths{ GetOtherPaths() };
    for( const AssetPathStringView assetPath : paths )
    {
      if( assetPath.ends_with( ".meta" ) )
        continue;

      ImGuiText( assetPath.GetFilename() );
    }
  }

  static void UIFilesModelImGui( const AssetPathStringView assetPath, Errors& errors )
  {
    TAC_ASSERT( !errors );

    const String filename{ assetPath.GetFileExtension() };
    AssetViewImportedModel* loadedModel { GetLoadedModel( assetPath ) };
    if( !loadedModel )
    {
      const ShortFixedString text{
        ShortFixedString::Concat( "Loading ", filename, LoadEllipses() ) };
      ImGuiText( text );
      return;
    }

    if( !loadedModel->mWorld.mEntities.empty() )
    {
      // this line should be removed, no? it should only compute transforms recursively, if even.
      loadedModel->mWorld.Step( TAC_DT );

      ImGuiImage( loadedModel->mTextureHandleColor.GetIndex(), v2( sAssetViewWidth, sAssetViewHeight ) );
      ImGuiSameLine();
      ImGuiBeginGroup();
      ImGuiText( filename );
      // this will select the entity in the loadedModel->mWorld, not gCreation.mWorld!!!
      // probably a super hack
      //if( ImGuiButton( "Select in property window" ) )
      //{
      //  gCreation.ClearSelection();
      //  gCreation.AddToSelection( *loadedModel->mWorld.mEntities.begin() );
      //}

      if( ImGuiButton( "Import object into scene" ) )
        Creation::InstantiateAsCopy( loadedModel->mPrefab );

      ImGuiEndGroup();
    }
    else if( loadedModel->mAttemptedToLoadEntity )
    {
      ImGuiText( ShortFixedString::Concat( assetPath, "has nothing to import" ) );
    }
    else
    {
      ImGuiText( ShortFixedString::Concat( "IDK what this code path is ", assetPath ) );
    }
  }

  static void UIFilesModels(  Errors& errors )
  {
    for( AssetPathStrings paths{ GetModelPaths() }; const AssetPathStringView path : paths )
      UIFilesModelImGui(  path, errors );
  }

  static void UIFilesImages( Errors& errors )
  {
    const AssetPathStrings paths{ GetImagePaths() };

    int shownImageCount {};
    bool goSameLine {};
    for( const AssetPathStringView assetPath : paths )
    {
      if( goSameLine )
        ImGuiSameLine();

      goSameLine = false;
      ImGuiBeginChild( assetPath, { 200, 100 } );
      ImGuiText( assetPath.GetFilename() );
      const Render::TextureHandle textureHandle{ TextureAssetManager::GetTexture( assetPath, sAssetViewErrors ) };
      if( textureHandle.IsValid() )
        ImGuiImage( textureHandle.GetIndex(), v2( 50, 50 ) );

      ImGuiEndChild();
      if( shownImageCount++ % 3 < 3 - 1 )
        goSameLine = true;
    }
  }

  static void UIFiles(  Errors& errors )
  {
    if( sAssetViewFiles.empty() )
      ImGuiText( "no files :(" );

    TAC_CALL( UIFilesImages( errors ) );
    TAC_CALL( UIFilesModels(  errors ) );
    TAC_CALL( UIFilesOther( errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  bool CreationAssetView::sShowWindow{};

  void CreationAssetView::Update( Errors& errors )
  {
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowMoveResize();
    if(  !ImGuiBegin( "Asset View" )  )
      return;
    TAC_PROFILE_BLOCK;
    ImGuiText( "--- Asset View ---" );
    if( sAssetViewErrors )
      ImGuiText( sAssetViewErrors.ToString() );

    const int oldStackSize { sAssetViewFolderStack.size() };

    if( sAssetViewFolderStack.empty() )
      sAssetViewFolderStack.push_back( AssetPathString( "assets" ) );

    UIFoldersUpToCurr();

    {
      ImGuiBeginGroup();
      TAC_CALL( UIFoldersNext( errors ) );
      ImGuiEndGroup();
    }

    ImGuiSameLine();

    ImGuiBeginGroup();
    TAC_CALL( UIFiles( errors ) );
    ImGuiEndGroup();

    if( oldStackSize != sAssetViewFolderStack.size() || ImGuiButton( "Refresh" ) )
    {
      sAssetViewFolderCur = Join( sAssetViewFolderStack, "/" );
      PopulateFolderContents( sAssetViewErrors );
    }
    ImGuiEnd();
  }

  void CreationAssetView::Render( Errors& errors )
  {
    if( !sShowWindow )
      return;

    const AssetPathStrings paths{ GetModelPaths() };
    for( const AssetPathStringView assetPath : paths )
    {
      AssetViewImportedModel* loadedModel{ GetLoadedModel( assetPath ) };
      if( !loadedModel )
      {
        TAC_CALL( loadedModel = CreateLoadedModel( assetPath, errors ) );
      }

      AssetImporterAttemptLoadEntity( loadedModel, assetPath, errors );
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      TAC_CALL( Render::IContext::Scope renderContextScope{
        renderDevice->CreateRenderContext( errors ) } );
      Render::IContext* renderContext{ renderContextScope.GetContext() };
      RenderImportedModel( renderContext, loadedModel, errors );
      TAC_CALL( renderContext->Execute( errors ) );
    }
  }

} // namespace Tac

