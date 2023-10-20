#include "src/common/assetmanagers/tac_model_load_synchronous.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/assetmanagers/tac_resident_model_file.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/system/tac_job_queue.h"
#include "src/common/math/tac_math.h"
#include "src/common/system/tac_os.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/string/tac_string.h"
#include "src/common/memory/tac_temporary_memory.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/thirdparty/cgltf.h"
#include "src/creation/tac_creation.h"
#include "src/creation/tac_creation_asset_view.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"
#include "src/space/model/tac_model.h"
#include "src/common/graphics/tac_camera.h"

#include <map>
//#include <cmath>

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


    // somethings are embedded in the model file
    // - mesh / material
    // - possible textures
    //
    // other things are stored in external files
    // - textures
  };

  static String                                       sAssetViewFolderCur;
  static Vector< String >                             sAssetViewFolderStack;
  static Errors                                       sAssetViewErrors;
  static Vector< String >                             sAssetViewFiles;
  static Vector< String >                             sAssetViewFolders;
  static std::map< String, AssetViewImportedModel* >  sLoadedModels;

  static String LoadEllipses() { return String( "...", ( int )ShellGetElapsedSeconds() % 4 ); }

  static AssetViewImportedModel* GetLoadedModel( const String& path )
  {
    auto loadedModelIt = sLoadedModels.find( path );
    return loadedModelIt == sLoadedModels.end() ? nullptr : ( *loadedModelIt ).second;
  }

  //static AssetViewImportingModel* GetLoadingModel( const String& path )
  //{
  //  auto loadingModelIt = sLoadingModels.find( path );
  //  return loadingModelIt == sLoadingModels.end() ? nullptr : ( *loadingModelIt ).second;
  //}

  static void PopulateFolderContents()
  {
    sAssetViewFiles.clear();
    sAssetViewFolders.clear();
    OS::OSGetFilesInDirectory( sAssetViewFiles,
                                    sAssetViewFolderCur,
                                    OSGetFilesInDirectoryFlags::Default,
                                    sAssetViewErrors );
    OS::OSGetDirectoriesInDirectory( sAssetViewFolders, sAssetViewFolderCur, sAssetViewErrors );
  }

  static void UIFoldersUpToCurr()
  {
    for( int iStack = 0; iStack < sAssetViewFolderStack.size(); ++iStack )
    {
      const String& folder = sAssetViewFolderStack[ iStack ];
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

  static void UIFoldersNext()
  {
    for( const String& path : sAssetViewFolders )
    {
      const int iPath = ( int )( &path - sAssetViewFolders.data() );
      if( ImGuiButton( path ) )
      {
        sAssetViewFolderStack.push_back( path );
      }
      if( iPath != sAssetViewFolders.size() - 1 )
        ImGuiSameLine();
    }
  }

  static bool HasExt( const StringView& path, Vector< const char* > extensions )
  {
    for( const char* ext : extensions )
      if( path.ends_with( ext ) )
        return true;
    return false;
  }

  static bool IsImage( const StringView& path ) { return HasExt( path, { ".png", ".jpg", ".bmp" } ); }

  static bool IsModel( const StringView& path ) { return HasExt( path, { ".gltf", ".glb" } ); }

  static void UIFilesOther( const Vector< String >& paths )
  {
    for( const String& path : paths )
      ImGuiText( SplitFilepath( path ).mFilename );
  }

  static void AttemptLoadEntity( AssetViewImportedModel* loadedModel, const char* path )
  {
    if( loadedModel->mAttemptedToLoadEntity )
      return;

    const cgltf_data* gltfData = TryGetGLTFData( path );
    if( !gltfData )
      return;

    TAC_ON_DESTRUCT( loadedModel->mAttemptedToLoadEntity = true );

    Entity* entityRoot = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
    entityRoot->mName = SplitFilepath( path ).mFilename;

    Vector< Entity* > entityNodes;
    for( int i = 0; i < (int)gltfData->nodes_count; ++i )
    {
      Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
      entityNodes.push_back( entityNode );
    }

    for( int i = 0; i < (int)gltfData->nodes_count; ++i )
    {
      const cgltf_node& node = gltfData->nodes[ i ];

      Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
      entityNode->mName = node.name ? node.name : va( "node %i", i );
      entityNode->mRelativeSpace = DecomposeGLTFTransform( &node );

      if( node.mesh )
      {
        const String modelPath = [ path ]()
        {
          const StringView initialWorkingDir = ShellGetInitialWorkingDir();
          String modelPath = path;
          if( modelPath.starts_with( initialWorkingDir ) )
            modelPath = modelPath.substr( initialWorkingDir.size() );
          while( modelPath.starts_with( '/' ) || modelPath.starts_with( '\\' ) )
            modelPath.erase( 0, 1 );
          return modelPath;
        }( );


        auto model = ( Model* )entityNode->AddNewComponent( Model().GetEntry() );
        model->mModelPath = modelPath;
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

  static void UIFilesModelImGui( const String& path )
  {
    const String filename = SplitFilepath( path ).mFilename;
    AssetViewImportedModel* loadedModel = GetLoadedModel( path );

    if( loadedModel )
    {
      if( loadedModel->mWorld.mEntities.size() )
      {
        ImGuiImage( ( int )loadedModel->mTextureHandleColor, v2( w, h ) );
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
        {
          Entity* prefab = *loadedModel->mWorld.mEntities.begin();
          gCreation.InstantiateAsCopy( prefab, gCreation.GetEditorCameraVisibleRelativeSpace() );
        }
        ImGuiEndGroup();
      }
      else if( loadedModel->mAttemptedToLoadEntity )
      {
        ImGuiText( filename + " has nothing to import" );
      }
      else
      {
        ImGuiText( "Loading " + filename + LoadEllipses() );
      }
    }
    else
    {
      ImGuiText( "Loading " + filename + LoadEllipses() );
    }
  }

  static void UIFilesModel( const String& path )
  {
    if( AssetViewImportedModel* loadedModel = GetLoadedModel( path ) )
    {
      AttemptLoadEntity( loadedModel, path.c_str() );
      if( loadedModel->mWorld.mEntities.size() )
      {
        const String filename = SplitFilepath( path ).mFilename;
        TAC_RENDER_GROUP_BLOCK( FrameMemoryPrintf( "asset preview %s", filename.c_str() ) );
        Render::SetViewFramebuffer( loadedModel->mViewHandle, loadedModel->mFramebufferHandle );
        Render::SetViewport( loadedModel->mViewHandle, Render::Viewport( w, h ) );
        Render::SetViewScissorRect( loadedModel->mViewHandle, Render::ScissorRect( w, h ) );
        GamePresentationRender( &loadedModel->mWorld,
                                &loadedModel->mCamera,
                                w, h,
                                loadedModel->mViewHandle );
      }
    }
    else
    {
      const char* debugName = FrameMemoryPrintf( "%s-framebuffer-",
                                                 StripExt( SplitFilepath( path ).mFilename ).c_str() );

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
      Render::SetRenderObjectDebugName( textureHandleColor, debugName );

      const Render::TexSpec texSpecDepth = 
      {
        .mImage =
        {
          .mWidth = w,
          .mHeight = h,
          .mFormat =
          {
            .mElementCount = 1,
            .mPerElementByteCount = sizeof( uint16_t ),
            .mPerElementDataType = Render::GraphicsType::unorm,
          },
        },
        .mBinding = Render::Binding::DepthStencil,
      };
      const Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( textureHandleDepth, debugName );

      const Render::FramebufferHandle framebufferHandle = Render::CreateFramebufferForRenderToTexture(
        { textureHandleColor, textureHandleDepth }, TAC_STACK_FRAME );
      Render::ViewHandle viewHandle = Render::CreateView();

      loadedModel = new AssetViewImportedModel;
      loadedModel->mFramebufferHandle = framebufferHandle;
      loadedModel->mTextureHandleColor = textureHandleColor;
      loadedModel->mTextureHandleDepth = textureHandleDepth;
      loadedModel->mViewHandle = viewHandle;
      sLoadedModels[ path ] = loadedModel;
    }
  }

  static void UIFilesModels( const Vector< String >& paths )
  {
    for( const String& path : paths )
    {
      UIFilesModel( path );
      UIFilesModelImGui( path );
    }
  }

  static void UIFilesImages( const Vector< String >& paths )
  {
    int shownImageCount = 0;
    bool goSameLine = false;
    for( const String& path : paths )
    {
      if( goSameLine )
        ImGuiSameLine();
      goSameLine = false;
      ImGuiBeginChild( path, { 200, 100 } );
      ImGuiText( SplitFilepath( path ).mFilename );
      const Render::TextureHandle textureHandle = TextureAssetManager::GetTexture( path, sAssetViewErrors );
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

    Vector< String > imagePaths;
    Vector< String > modelPaths;
    Vector< String > otherPaths;

    for( const String& path : sAssetViewFiles )
    {
      Vector< String >* paths = &otherPaths;
      if( IsImage( path ) )
        paths = &imagePaths;
      if( IsModel( path ) )
        paths = &modelPaths;
      ( *paths ).push_back( path );
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
        const String root = ShellGetInitialWorkingDir() + String( "/assets" );
        sAssetViewFolderStack.push_back( root );
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

}

