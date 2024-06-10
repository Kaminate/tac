#include "tac_level_editor_asset_view.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_model_load_synchronous.h"
#include "tac-engine-core/assetmanagers/gltf/tac_resident_model_file.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
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
    bool                      mAttemptedToLoadEntity { false };
    EntityUUIDCounter         mEntityUUIDCounter;
    World                     mWorld;
    Render::TextureHandle     mTextureHandleColor;
    Render::TextureHandle     mTextureHandleDepth;
    Camera                    mCamera;
    AssetPathString           mAssetPath;
    Debug3DDrawBuffers        mDebug3DDrawBuffers;
  };


  using LoadedModels = Vector< AssetViewImportedModel* >;

  static AssetPathString   sAssetViewFolderCur;
  static Vector< String >  sAssetViewFolderStack;
  static Errors            sAssetViewErrors;
  static FileSys::Paths    sAssetViewFiles;
  static FileSys::Paths    sAssetViewFolders;
  static LoadedModels      sLoadedModels;
  static const int         w { 256 };
  static const int         h { 256 };

  static String LoadEllipses() { return String( "...", ( int )Timestep::GetElapsedTime() % 4 ); }

  static bool HasExt( const FileSys::Path& path, Vector< const char* > extensions )
  {
    for( const char* ext : extensions )
      if( path.u8string().ends_with( ext ) )
        return true;

    return false;
  }

  static bool IsImage( const FileSys::Path& path )
  {
    Vector< const char* > exts;
    exts.push_back( ".png" );
    exts.push_back( ".jpg" );
    exts.push_back( ".bmp" );
    return HasExt( path, exts );
  }

  static bool IsModel( const FileSys::Path& path )
  {
    Vector< const char* > exts;
    exts.push_back( ".gltf" );
    exts.push_back( ".glb" );
    return HasExt( path, exts );
  }


  static FileSys::Paths GetImagePaths()
  {
    FileSys::Paths imagePaths;
    for( const FileSys::Path& path : sAssetViewFiles )
      if( IsImage( path ) )
        imagePaths.push_back( path );

    return imagePaths;
  }

  static FileSys::Paths GetModelPaths()
  {
    FileSys::Paths modelPaths;
    for( const FileSys::Path& path : sAssetViewFiles )
      if( IsModel( path ) )
        modelPaths.push_back( path );

    return modelPaths;
  }

  static FileSys::Paths GetOtherPaths()
  {
    FileSys::Paths otherPaths;
    for( const FileSys::Path& path : sAssetViewFiles )
      if( !IsImage( path ) && !IsModel( path ) )
        otherPaths.push_back( path );

    return otherPaths;
  }

  static AssetViewImportedModel* GetLoadedModel( const AssetPathStringView& path )
  {
    for( AssetViewImportedModel* model : sLoadedModels )
      if( ( StringView )model->mAssetPath == path )
        return model;

    return nullptr;
  }

  static RelativeSpace DecomposeGLTFTransform( const cgltf_node* node )
  {
    cgltf_float local[ 16 ];
    cgltf_node_transform_local( node, local );

    m4 mLocal;
    for( int i { 0 }; i < 16; ++i )
      mLocal[ i ] = local[ i ];
    mLocal.Transpose();

    v3 c0 = { mLocal.m00, mLocal.m10, mLocal.m20 };
    v3 c1 = { mLocal.m01, mLocal.m11, mLocal.m21 };
    v3 c2 = { mLocal.m02, mLocal.m12, mLocal.m22 };
    v3 c3 = { mLocal.m03, mLocal.m13, mLocal.m23 };
    v3 scale = { Length( c0 ), Length( c1 ), Length( c2 ) };
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
    float zPhi { 0 };
    float yTheta { 0 };
    float xPsi { 0 };


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
    const RelativeSpace relativeSpace
    {
      .mPosition = c3,
      .mEulerRads = eulerRads,
      .mScale = scale,
    };
    return relativeSpace;
  }

  static void PopulateFolderContents()
  {
    //sAssetViewFiles.clear();
    //sAssetViewFolders.clear();
    sAssetViewFiles = FileSys::IterateFiles( sAssetViewFolderCur,
                                                FileSys::IterateType::Default,
                                                sAssetViewErrors );
    if( sAssetViewErrors )
      return;

    //OS::OSGetFilesInDirectory( sAssetViewFiles,
    //                                sAssetViewFolderCur,
    //                                OS::OSGetFilesInDirectoryFlags::Default,
    //                                sAssetViewErrors );
    sAssetViewFolders = FileSys::IterateDirectories( sAssetViewFolderCur,
                                                        FileSys::IterateType::Default,
                                                        sAssetViewErrors );
    if( sAssetViewErrors )
      return;
    //OS::OSGetDirectoriesInDirectory( sAssetViewFolders, sAssetViewFolderCur, sAssetViewErrors );
  }

  static void UIFoldersUpToCurr()
  {
    for( int iStack { 0 }; iStack < sAssetViewFolderStack.size(); ++iStack )
    {
      const FileSys::Path& folder { sAssetViewFolderStack[ iStack ] };
      if( ImGuiButton( folder.u8string() ) )
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

  static void UIFoldersNext()
  {
    const int n { sAssetViewFolders.size() };
    for( int i { 0 }; i < n; ++i )
    {
      const FileSys::Path& path { sAssetViewFolders[ i ] };
      const int iPath { i };
      if( ImGuiButton( path.u8string() ) )
        sAssetViewFolderStack.push_back( path.u8string() );
      if( iPath != sAssetViewFolders.size() - 1 )
        ImGuiSameLine();
    }
  }


  static void UIFilesOther()
  {
    FileSys::Paths paths{ GetOtherPaths() };
    for( const FileSys::Path& path : paths )
      ImGuiText( path.u8string() );
  }

  static void AttemptLoadEntity( AssetViewImportedModel* loadedModel,
                                 const AssetPathStringView& path )
  {
    if( loadedModel->mAttemptedToLoadEntity )
      return;

    const cgltf_data* gltfData { TryGetGLTFData( path ) };
    if( !gltfData )
      return;

    TAC_ON_DESTRUCT( loadedModel->mAttemptedToLoadEntity = true );

    Entity* entityRoot { loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() ) };
    entityRoot->mName = path;// Filesystem::FilepathToFilename( path );

    Vector< Entity* > entityNodes;
    for( int i { 0 }; i < (int)gltfData->nodes_count; ++i )
    {
      Entity* entityNode { loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() ) };
      entityNodes.push_back( entityNode );
    }

    for( int i { 0 }; i < (int)gltfData->nodes_count; ++i )
    {
      const cgltf_node& node { gltfData->nodes[ i ] };

      String name { node.name };
      if( name.empty() )
      {
        name += "node ";
        name += ToString( i );
      }

      Entity* entityNode { loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() ) };
      entityNode->mName = name;
      entityNode->mRelativeSpace = DecomposeGLTFTransform( &node );

      if( node.mesh )
      {
        //const String modelPath = [ path ]()
        //{
        //  const FileSys::Path& initialWorkingDir = ShellGetInitialWorkingDir();
        //  String modelPath = path;
        //  if( modelPath.starts_with( initialWorkingDir ) )
        //    modelPath = modelPath.substr( initialWorkingDir.size() );
        //  while( modelPath.starts_with( '/' ) || modelPath.starts_with( '\\' ) )
        //    modelPath.erase( 0, 1 );
        //  return modelPath;
        //}( );


        auto model { ( Model* )entityNode->AddNewComponent( Model().GetEntry() ) };
        model->mModelPath = path;
        //model->mModelName = node.mesh->name;
        model->mModelIndex = ( int )( node.mesh - gltfData->meshes );
        //model->mTryingNewThing = true;


        // ** REMEMBER **, we don't care about the textures on this model component.
        // that's a job for the material component
      }

      if( node.children_count )
      {
        for( cgltf_node* child { *node.children }; child < *node.children + node.children_count; ++child )
        {
          Entity* childEntity { entityNodes[ ( int )( child - *node.children ) ] };
          entityNode->AddChild( childEntity );
        }
      }

      if( !node.parent )
        entityRoot->AddChild( entityNode );
    }
  }

  static void UIFilesModelImGui( const FileSys::Path& path )
  {
    Errors errors;
    AssetPathStringView assetPath { ModifyPathRelative( path, errors ) };
    TAC_ASSERT( !errors );

    //const String filename = Filesystem::FilepathToFilename( path );
    AssetViewImportedModel* loadedModel { GetLoadedModel( assetPath ) };
    if( !loadedModel )
    {
      const ShortFixedString text{ ShortFixedString::Concat( "Loading ",
                                                              assetPath,
                                                              LoadEllipses() ) };

      ImGuiText( text );
      return;
    }

    if( loadedModel->mWorld.mEntities.size() )
    {
      ImGuiImage( loadedModel->mTextureHandleColor.GetIndex(), v2( w, h ) );
      ImGuiSameLine();
      ImGuiBeginGroup();
      ImGuiText( assetPath );
      // this will select the entity in the loadedModel->mWorld, not gCreation.mWorld!!!
      // probably a super hack
      //if( ImGuiButton( "Select in property window" ) )
      //{
      //  gCreation.ClearSelection();
      //  gCreation.AddToSelection( *loadedModel->mWorld.mEntities.begin() );
      //}

      if( ImGuiButton( "Import object into scene" ) )
      {
        Entity* prefab { *loadedModel->mWorld.mEntities.begin() };
        const RelativeSpace relativeSpace{ gCreation.GetEditorCameraVisibleRelativeSpace() };
        gCreation.InstantiateAsCopy( prefab, relativeSpace );
      }
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

  static Render::CreateTextureParams GetTexColorParams()
  {
    const Render::Image colorImg
    {
      .mWidth  { w },
      .mHeight { h },
      .mFormat { Render::TexFmt::kRGBA8_unorm }
    };
    const Render::CreateTextureParams texSpecColor
    {
      .mImage   { colorImg },
      .mBinding { Render::Binding::ShaderResource | Render::Binding::RenderTarget },
    };
    return texSpecColor;
  }

  static Render::CreateTextureParams GetTexDepthParams()
  {
    const Render::Image depthImg
    {
      .mWidth  { w },
      .mHeight { h },
      .mFormat { Render::TexFmt::kD24S8 }
    };
    const Render::CreateTextureParams texSpecDepth
    {
      .mImage    { depthImg },
      .mBinding  { Render::Binding::DepthStencil },
    };
    return texSpecDepth;
  }

  static AssetViewImportedModel* CreateLoadedModel( const AssetPathStringView& assetPath,
                                                    Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::CreateTextureParams colorParams{ GetTexColorParams() };
    TAC_CALL_RET( {}, const Render::TextureHandle textureHandleColor{
      renderDevice->CreateTexture( colorParams, errors ) } );

    const Render::CreateTextureParams depthParams{ GetTexDepthParams() };
    TAC_CALL_RET( {}, const Render::TextureHandle textureHandleDepth{
      renderDevice->CreateTexture( depthParams, errors ) } );

    AssetViewImportedModel* result{ TAC_NEW AssetViewImportedModel
    {
       .mTextureHandleColor { textureHandleColor },
       .mTextureHandleDepth { textureHandleDepth },
       .mAssetPath          { assetPath },
    } };

    sLoadedModels.push_back( result );
    return result;
  }

  static void RenderImportedModel( Render::IContext* renderContext,
                                   AssetViewImportedModel* loadedModel, Errors& errors )
  {
    if( loadedModel->mWorld.mEntities.empty() )
      return;

    const v2i viewSize{ w, h };
    const ShortFixedString groupName{ ShortFixedString::Concat( "asset preview ",
                            loadedModel->mAssetPath.c_str() ) };

    TAC_RENDER_GROUP_BLOCK( renderContext, groupName );

    const Render::Targets renderTargets
    {
      .mColors{ loadedModel->mTextureHandleColor },
      .mDepth{ loadedModel->mTextureHandleDepth },
    };
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    GamePresentationRender( &loadedModel->mWorld,
                            &loadedModel->mCamera,
                            viewSize,
                            loadedModel->mTextureHandleColor,
                            loadedModel->mTextureHandleDepth,
                            &loadedModel->mDebug3DDrawBuffers,
                            errors );
  }



  static void UIFilesModels()
  {
    FileSys::Paths paths{ GetModelPaths() };
    for( const FileSys::Path& path : paths )
    {
      UIFilesModelImGui( path );
    }
  }

  static void UIFilesImages()
  {
    FileSys::Paths paths{ GetImagePaths() };

    int shownImageCount {};
    bool goSameLine {};
    for( const FileSys::Path& path : paths )
    {
      if( goSameLine )
        ImGuiSameLine();

      goSameLine = false;

      const String pathUTF8 { path.u8string() };

      ImGuiBeginChild( pathUTF8, { 200, 100 } );

      ImGuiText(  pathUTF8 );
      const Render::TextureHandle textureHandle {
        TextureAssetManager::GetTexture( pathUTF8, sAssetViewErrors ) };
      if( textureHandle.IsValid() )
        ImGuiImage( textureHandle.GetIndex(), v2( 50, 50 ) );

      ImGuiEndChild();

      if( shownImageCount++ % 3 < 3 - 1 )
        goSameLine = true;
    }
  }

  static void UIFiles()
  {
    if( sAssetViewFiles.empty() )
      ImGuiText( "no files :(" );

    UIFilesImages();
    UIFilesModels();
    UIFilesOther();
  }


} // namespace Tac

void Tac::CreationUpdateAssetView()
{
  TAC_PROFILE_BLOCK;
  ImGuiSetNextWindowStretch();
  ImGuiSetNextWindowMoveResize();
  const bool open { ImGuiBegin( "Asset View" ) };
  if( open )
  {
    ImGuiText( "--- Asset View ---" );
    if( sAssetViewErrors )
      ImGuiText( sAssetViewErrors.ToString() );

    const int oldStackSize = sAssetViewFolderStack.size();

    if( sAssetViewFolderStack.empty() )
    {
      //const String root = "assets";
      //const FileSys::Path root = ShellGetInitialWorkingDir() / "assets";
      sAssetViewFolderStack.push_back( "assets" );
    }

    UIFoldersUpToCurr();
    UIFoldersNext();
    UIFiles();

    if( oldStackSize != sAssetViewFolderStack.size() || ImGuiButton( "Refresh" ) )
    {
      sAssetViewFolderCur = Join( sAssetViewFolderStack, "/" );
      PopulateFolderContents();
    }
  }
  ImGuiEnd();
}

void Tac::CreationRenderAssetView( Errors& errors )
{
  const FileSys::Paths paths{ GetModelPaths() };
  for( const FileSys::Path& path : paths )
  {
    TAC_CALL( AssetPathString assetPath{ ModifyPathRelative( path, errors ) } );

    AssetViewImportedModel* loadedModel{ GetLoadedModel( assetPath ) };
    if( !loadedModel )
    {
      TAC_CALL( loadedModel = CreateLoadedModel( assetPath, errors ) );
    }

    AttemptLoadEntity( loadedModel, assetPath );

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( Render::IContext::Scope renderContext{
      renderDevice->CreateRenderContext( errors ) } );

    RenderImportedModel( renderContext, loadedModel, errors );

    TAC_CALL( renderContext->Execute( errors ) );
  }
}

