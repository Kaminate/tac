#include "tac_level_editor_asset_view.h" // self-inc

#include "src/common/assetmanagers/gltf/tac_model_load_synchronous.h"
#include "src/common/assetmanagers/gltf/tac_resident_model_file.h"
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/containers/tac_map.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/math/tac_math.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timestep.h"
#include "src/common/string/tac_string.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_job_queue.h"
#include "src/common/system/tac_os.h"
#include "src/common/assetmanagers/gltf/tac_gltf.h"
#include "src/level_editor/tac_level_editor.h"
#include "space/graphics/model/tac_model.h"
#include "space/presentation/tac_game_presentation.h"
#include "space/ecs/tac_entity.h"
#include "space/world/tac_world.h"

namespace Tac
{
  static RelativeSpace DecomposeGLTFTransform( const cgltf_node* node )
  {
    cgltf_float local[ 16 ];
    cgltf_node_transform_local( node, local );

    m4 mLocal;
    for( int i = 0; i < 16; ++i )
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
    float zPhi = 0;
    float yTheta = 0;
    float xPsi = 0;


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

    RelativeSpace relativeSpace;
    relativeSpace.mEulerRads = { xPsi, yTheta, zPhi };
    relativeSpace.mPosition = c3;
    relativeSpace.mScale = scale;
    return relativeSpace;
  }

  struct AssetViewImportedModel
  {
    bool                      mAttemptedToLoadEntity = false;
    EntityUUIDCounter         mEntityUUIDCounter;
    World                     mWorld;
    // EditorLoadInfo            mLoadInfo;
    Render::TextureHandle     mTextureHandleColor;
    Render::TextureHandle     mTextureHandleDepth;
    Render::FramebufferHandle mFramebufferHandle;
    Render::ViewHandle        mViewHandle;
    Camera                    mCamera;
    AssetPathString           mAssetPath;


    // somethings are embedded in the model file
    // - mesh / material
    // - possible textures
    //
    // other things are stored in external files
    // - textures
  };

  static AssetPathString                                 sAssetViewFolderCur;
  static Vector< String >                                sAssetViewFolderStack;
  static Errors                                          sAssetViewErrors;
  static Filesystem::Paths                               sAssetViewFiles;
  static Filesystem::Paths                               sAssetViewFolders;


  static Map< AssetPathString, AssetViewImportedModel* > sLoadedModels;

  static String LoadEllipses() { return String( "...", ( int )Timestep::GetElapsedTime() % 4 ); }

  static AssetViewImportedModel* GetLoadedModel( const AssetPathStringView& path )
  {
    auto loadedModelIt = sLoadedModels.Find( path );
    return loadedModelIt ? loadedModelIt.GetValue() : nullptr;
  }

  //static AssetViewImportingModel* GetLoadingModel( const String& path )
  //{
  //  auto loadingModelIt = sLoadingModels.find( path );
  //  return loadingModelIt == sLoadingModels.end() ? nullptr : ( *loadingModelIt ).second;
  //}

  static void PopulateFolderContents()
  {
    //sAssetViewFiles.clear();
    //sAssetViewFolders.clear();
    sAssetViewFiles = Filesystem::IterateFiles( sAssetViewFolderCur,
                                                Filesystem::IterateType::Default,
                                                sAssetViewErrors );
    if( sAssetViewErrors )
      return;

    //OS::OSGetFilesInDirectory( sAssetViewFiles,
    //                                sAssetViewFolderCur,
    //                                OS::OSGetFilesInDirectoryFlags::Default,
    //                                sAssetViewErrors );
    sAssetViewFolders = Filesystem::IterateDirectories( sAssetViewFolderCur,
                                                        Filesystem::IterateType::Default,
                                                        sAssetViewErrors );
    if( sAssetViewErrors )
      return;
    //OS::OSGetDirectoriesInDirectory( sAssetViewFolders, sAssetViewFolderCur, sAssetViewErrors );
  }

  static void UIFoldersUpToCurr()
  {
    for( int iStack = 0; iStack < sAssetViewFolderStack.size(); ++iStack )
    {
      const Filesystem::Path& folder = sAssetViewFolderStack[ iStack ];
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
    const int n = sAssetViewFolders.size();
    for( int i = 0; i < n; ++i )
    {
      const Filesystem::Path& path = sAssetViewFolders[ i ];
      const int iPath = i;
      if( ImGuiButton( path.u8string() ) )
        sAssetViewFolderStack.push_back( path.u8string() );
      if( iPath != sAssetViewFolders.size() - 1 )
        ImGuiSameLine();
    }
  }

  static bool HasExt( const Filesystem::Path& path, Vector< const char* > extensions )
  {
    for( const char* ext : extensions )
      if( path.u8string().ends_with( ext ) )
        return true;
    return false;
  }

  static bool IsImage( const Filesystem::Path& path )
  {
    return HasExt( path, { ".png", ".jpg", ".bmp" } );
  }

  static bool IsModel( const Filesystem::Path& path )
  {
    return HasExt( path, { ".gltf", ".glb" } );
  }

  static void UIFilesOther( const Filesystem::Paths& paths )
  {
    for( const Filesystem::Path& path : paths )
      ImGuiText( path.u8string() );
  }

  static void AttemptLoadEntity( AssetViewImportedModel* loadedModel, const AssetPathStringView& path )
  {
    if( loadedModel->mAttemptedToLoadEntity )
      return;

    const cgltf_data* gltfData = TryGetGLTFData( path );
    if( !gltfData )
      return;

    TAC_ON_DESTRUCT( loadedModel->mAttemptedToLoadEntity = true );

    Entity* entityRoot = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
    entityRoot->mName = path;// Filesystem::FilepathToFilename( path );

    Vector< Entity* > entityNodes;
    for( int i = 0; i < (int)gltfData->nodes_count; ++i )
    {
      Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
      entityNodes.push_back( entityNode );
    }

    for( int i = 0; i < (int)gltfData->nodes_count; ++i )
    {
      const cgltf_node& node = gltfData->nodes[ i ];

      String name = node.name;
      if( name.empty() )
      {
        name += "node ";
        name += ToString( i );
      }

      Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
      entityNode->mName = name;
      entityNode->mRelativeSpace = DecomposeGLTFTransform( &node );

      if( node.mesh )
      {
        //const String modelPath = [ path ]()
        //{
        //  const Filesystem::Path& initialWorkingDir = ShellGetInitialWorkingDir();
        //  String modelPath = path;
        //  if( modelPath.starts_with( initialWorkingDir ) )
        //    modelPath = modelPath.substr( initialWorkingDir.size() );
        //  while( modelPath.starts_with( '/' ) || modelPath.starts_with( '\\' ) )
        //    modelPath.erase( 0, 1 );
        //  return modelPath;
        //}( );


        auto model = ( Model* )entityNode->AddNewComponent( Model().GetEntry() );
        model->mModelPath = path;
        //model->mModelName = node.mesh->name;
        model->mModelIndex = ( int )( node.mesh - gltfData->meshes );
        //model->mTryingNewThing = true;


        // ** REMEMBER **, we don't care about the textures on this model component.
        // that's a job for the material component
      }

      if( node.children_count )
      {
        for( cgltf_node* child = *node.children; child < *node.children + node.children_count; ++child )
        {
          Entity* childEntity = entityNodes[ ( int )( child - *node.children ) ];
          entityNode->AddChild( childEntity );
        }
      }

      if( !node.parent )
        entityRoot->AddChild( entityNode );
    }
  }

  const int w = 256;
  const int h = 256;

  static void UIFilesModelImGui( const Filesystem::Path& path )
  {
    Errors errors;
    AssetPathStringView assetPath = ModifyPathRelative( path, errors );
    TAC_ASSERT( !errors );

    //const String filename = Filesystem::FilepathToFilename( path );
    AssetViewImportedModel* loadedModel = GetLoadedModel( assetPath );
    if( !loadedModel )
    {
      const ShortFixedString text = ShortFixedString::Concat( "Loading ",
                                                              assetPath,
                                                              LoadEllipses() );

      ImGuiText(text);
      return;
    }

    if( loadedModel->mWorld.mEntities.size() )
    {
      ImGuiImage( ( int )loadedModel->mTextureHandleColor, v2( w, h ) );
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
        Entity* prefab = *loadedModel->mWorld.mEntities.begin();
        gCreation.InstantiateAsCopy( prefab, gCreation.GetEditorCameraVisibleRelativeSpace() );
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

  static AssetViewImportedModel* CreateLoadedModel(const AssetPathStringView& assetPath)
  {
    const Render::TexSpec texSpecColor =
    {
      .mImage =
      {
        .mWidth = w,
        .mHeight = h,
        .mFormat =
        {
          .mElementCount = 4,
          .mPerElementByteCount = 1,
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mBinding = Render::Binding::ShaderResource | Render::Binding::RenderTarget,
    };
    const Render::TextureHandle textureHandleColor = Render::CreateTexture( texSpecColor, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( textureHandleColor, assetPath.GetFilename() );

    const Render::TexSpec texSpecDepth = 
    {
      .mImage =
      {
        .mWidth = w,
        .mHeight = h,
        .mFormat =
        {
          .mElementCount = 1,
          .mPerElementByteCount = sizeof( u16 ),
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mBinding = Render::Binding::DepthStencil,
    };
    const Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( textureHandleDepth, assetPath.GetFilename() );

    const Render::FramebufferHandle framebufferHandle = Render::CreateFramebufferForRenderToTexture(
      { textureHandleColor, textureHandleDepth }, TAC_STACK_FRAME );
    Render::ViewHandle viewHandle = Render::CreateView();

    auto* result = new AssetViewImportedModel
    {
       .mTextureHandleColor = textureHandleColor,
       .mTextureHandleDepth = textureHandleDepth,
       .mFramebufferHandle = framebufferHandle,
       .mViewHandle = viewHandle,
       .mAssetPath = assetPath,
    };

    sLoadedModels[ assetPath ] = result;
    return result;
  }

  static void RenderImportedModel( AssetViewImportedModel* loadedModel )
  {
    if( loadedModel->mWorld.mEntities.empty() )
      return;

    TAC_RENDER_GROUP_BLOCK( ShortFixedString::Concat( "asset preview ",
                            loadedModel->mAssetPath.c_str() ) );
    Render::SetViewFramebuffer( loadedModel->mViewHandle, loadedModel->mFramebufferHandle );
    Render::SetViewport( loadedModel->mViewHandle, Render::Viewport( w, h ) );
    Render::SetViewScissorRect( loadedModel->mViewHandle, Render::ScissorRect( w, h ) );
    GamePresentationRender( &loadedModel->mWorld,
                            &loadedModel->mCamera,
                            w, h,
                            loadedModel->mViewHandle );
  }

  static void UIFilesModel( const Filesystem::Path& path )
  {
    Errors errors;
    AssetPathString assetPath = ModifyPathRelative( path, errors );
    TAC_ASSERT( !errors );

    AssetViewImportedModel* loadedModel = GetLoadedModel( assetPath );
    if( !loadedModel )
      loadedModel = CreateLoadedModel( assetPath );

    AttemptLoadEntity( loadedModel, assetPath );

    RenderImportedModel( loadedModel );
  }

  static void UIFilesModels( const Filesystem::Paths& paths )
  {
    for( const Filesystem::Path& path : paths )
    {
      UIFilesModel( path );
      UIFilesModelImGui( path );
    }
  }

  static void UIFilesImages( const Filesystem::Paths& paths )
  {
    int shownImageCount = 0;
    bool goSameLine = false;
    for( const Filesystem::Path& path : paths )
    {
      if( goSameLine )
        ImGuiSameLine();
      goSameLine = false;

      const String pathUTF8 = path.u8string();

      ImGuiBeginChild( pathUTF8, { 200, 100 } );
      ImGuiText(  pathUTF8 );
      const Render::TextureHandle textureHandle = TextureAssetManager::GetTexture( pathUTF8, sAssetViewErrors );
      if( textureHandle.IsValid() )
        ImGuiImage( ( int )textureHandle, v2( 50, 50 ) );
      ImGuiEndChild();

      if( shownImageCount++ % 3 < 3 - 1 )
        goSameLine = true;
    }
  }

  static void UIFiles()
  {
    if( sAssetViewFiles.empty() )
      ImGuiText( "no files :(" );

    Filesystem::Paths imagePaths;
    Filesystem::Paths modelPaths;
    Filesystem::Paths otherPaths;

    for( const Filesystem::Path& path : sAssetViewFiles )
    {
      if( IsImage( path ) )
        imagePaths.push_back( path );
      else if( IsModel( path ) )
        modelPaths.push_back( path );
      else
        otherPaths.push_back( path );
    }

    UIFilesImages( imagePaths );
    UIFilesModels( modelPaths );
    UIFilesOther( otherPaths );
  }

  void CreationUpdateAssetView()
  {
    TAC_PROFILE_BLOCK;
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowMoveResize();
    const bool open = ImGuiBegin( "Asset View" );
    if( open )
    {
      ImGuiText( "--- Asset View ---" );
      if( sAssetViewErrors )
        ImGuiText( sAssetViewErrors.ToString() );

      const int oldStackSize = sAssetViewFolderStack.size();

      if( sAssetViewFolderStack.empty() )
      {
        //const String root = "assets";
        //const Filesystem::Path root = ShellGetInitialWorkingDir() / "assets";
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

} // namespace Tac

