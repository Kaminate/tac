#include "src/common/assetmanagers/tacModelLoadSynchronous.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacOS.h"
#include "src/common/tacShell.h"
#include "src/common/tacShellTimer.h"
#include "src/common/string/tacString.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacUtility.h"
#include "src/common/thirdparty/cgltf.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationAssetView.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/tacentity.h"
#include "src/space/tacworld.h"
#include "src/space/model/tacmodel.h"
#include "src/common/tacCamera.h"

#include <map>

            //GamePresentation gp;

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
      yTheta = -std::asin( r31 );
      xPsi = std::atan2( r32 / std::cos( yTheta ), r33 / std::cos( yTheta ) );
      zPhi = std::atan2( r21 / std::cos( yTheta ), r11 / std::cos( yTheta ) );
    }
    else
    {
      zPhi = 0;
      if( r31 == -1.0f )
      {
        yTheta = 3.14f / 2.0f;
        xPsi = std::atan2( r12, r13 );
      }
      else
      {
        yTheta = -3.14f / 2.0f;
        xPsi = std::atan2( -r12, -r13 );
      }
    }

    RelativeSpace relativeSpace;
    relativeSpace.mEulerRads = { xPsi, yTheta, zPhi };
    relativeSpace.mPosition = c3;
    relativeSpace.mScale = scale;
    return relativeSpace;
  }

  struct EditorLoadInfo
  {
    void      Init( const char* path, Errors&  errors )
    {
      bytes = TemporaryMemoryFromFile( path, errors );
      TAC_HANDLE_ERROR( errors );

      const cgltf_options options = {};

      const cgltf_result parseResult = cgltf_parse( &options, bytes.data(), bytes.size(), &parsedData );
      TAC_HANDLE_ERROR_IF( parseResult != cgltf_result_success,
                           va( "%s cgltf_parse returned %i", path, ( int )( parseResult ) ),
                           errors );

      const cgltf_result validateResult = cgltf_validate( parsedData );
      TAC_HANDLE_ERROR_IF( validateResult != cgltf_result_success,
                           va( "%s cgltf_validate returned %i", path, ( int )( validateResult ) ),
                           errors );

    }

    ~EditorLoadInfo()
    {
      cgltf_free( parsedData );
    }

    cgltf_data*     parsedData = nullptr;
    TemporaryMemory bytes;
  };


  //IEditorLoadInfo* LoadEditorModelFileInfo( const StringView& path, Errors& errors )
  //{

  struct AssetViewImportModelJob : Job
  {
    void Execute() override
    {
      Tac::Errors& errors = mErrors;
      TAC_UNUSED_PARAMETER( errors );

        //auto memory = TemporaryMemoryFromFile( mData->mFilepath, errors );
        //TAC_HANDLE_ERROR( errors );

        //mData->mPitch = pitch;
    }

    String mPath;
  };


  struct AssetViewImportingModel
  {
    void Update()
    {
      mJob.GetStatus();

      JobState::JustBeenCreated;
      JobState::ThreadQueued;
      JobState::ThreadRunning;
      JobState::ThreadFinished;
    }

    AssetViewImportModelJob mJob;
    //AssetViewImportModelJob* mJob;
  };

  struct AssetViewImportedModel
  {
    EntityUUIDCounter         mEntityUUIDCounter;
    World                     mWorld;
    EditorLoadInfo            mLoadInfo;
    Render::TextureHandle     mTextureHandleColor;
    Render::TextureHandle     mTextureHandleDepth;
    Render::FramebufferHandle mFramebufferHandle;
    Render::ViewHandle        mViewHandle;

    // somethings are embedded in the model file
    // - mesh / material
    // - possible textures
    // other things are stored in external files
    // - textures
  };

  static String           sAssetViewFolderCur;
  static Vector< String > sAssetViewFolderStack;
  static Errors           sAssetViewErrors;
  static Vector< String > sAssetViewFiles;
  static Vector< String > sAssetViewFolders;

  //static World            sAssetViewWorld;

  static std::map< String, AssetViewImportedModel* >  sLoadedModels;
  static std::map< String, AssetViewImportingModel* > sLoadingModels;

  static String LoadElipses() { return String( "...", ( int )ShellGetElapsedSeconds() % 4 ); }

  static AssetViewImportedModel* GetLoadedModel( const String& path )
  {
    auto loadedModelIt = sLoadedModels.find( path );
    return loadedModelIt == sLoadedModels.end() ? nullptr : ( *loadedModelIt ).second;
  }

  static AssetViewImportingModel* GetLoadingModel( const String& path )
  {
    auto loadingModelIt = sLoadingModels.find( path );
    return loadingModelIt == sLoadingModels.end() ? nullptr : ( *loadingModelIt ).second;
  }

  static void PopulateFolderContents()
  {
    sAssetViewFiles.clear();
    sAssetViewFolders.clear();
    OSGetFilesInDirectory( sAssetViewFiles, sAssetViewFolderCur, OSGetFilesInDirectoryFlags::Default, sAssetViewErrors );
    OSGetDirectoriesInDirectory( sAssetViewFolders, sAssetViewFolderCur, sAssetViewErrors );
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

    for( const String& path : imagePaths )
    {
      const int iPath = ( int )( &path - imagePaths.data() );
      const bool isImage =
        StringView( path ).ends_with( ".png" ) ||
        StringView( path ).ends_with( ".jpg" ) ||
        StringView( path ).ends_with( ".bmp" ) ||
        false;
      if( isImage )
        ImGuiBeginChild( path, { 200, 100 } );
      ImGuiText( SplitFilepath( path ).mFilename );

      if( isImage )
      {
        const Render::TextureHandle textureHandle = TextureAssetManager::GetTexture( path, sAssetViewErrors );
        if( textureHandle.IsValid() )
        {
          ImGuiImage( ( int )textureHandle, v2( 50, 50 ) );
        }
      }

      if( isImage )
        ImGuiEndChild();
      if( iPath % 3 < 3 - 1 && iPath != imagePaths.size() - 1 )
        ImGuiSameLine();
    }

    for( const String& path : modelPaths )
    {
      const String filename = SplitFilepath( path ).mFilename;
      //const String filenamePrefab = StripExt( filename ) + ".prefab";
      ImGuiText( filename );
      //
      //bool importFound = false;
      //for( const String& assetViewFile : sAssetViewFiles )
      //  if( SplitFilepath( assetViewFile ).mFilename == filenamePrefab )
      //    importFound = true;
      //if( !importFound )
      //{
      //  ImGuiSameLine();
      //  ImGuiButton( "Import" );
      //}

      if( AssetViewImportedModel* loadedModel = GetLoadedModel( path ) )
      {
        ImGuiSameLine();
        ImGuiButton( "Loaded" );

        ImGuiSameLine();
        if( ImGuiButton( "Import object into scene" ) )
        {
          //Entity* entity = gCreation.CreateEntity();
          //v3 pos = entity->mRelativeSpace.mPosition;
          //TAC_UNUSED_PARAMETER( entity );

          Entity* prefab = *loadedModel->mWorld.mEntities.begin();

          gCreation.InstantiateAsCopy( prefab, gCreation.GetEditorCameraVisibleRelativeSpace() );
          //entity->DeepCopy( **loadedModel->mWorld.mEntities.begin() );
          //entity->mRelativeSpace.mPosition = pos;
        }

        //ImGuiSameLine();
        //ImGuiText( va( "mesh count %i", loadedModel->mLoadInfo.parsedData->meshes_count ) );

        //ImGuiSameLine();
        //ImGuiText( va( "mat count %i", loadedModel->mLoadInfo.parsedData->materials_count ) );

        //ImGuiSameLine();
        //ImGuiText( va( "tex count %i", loadedModel->mLoadInfo.parsedData->textures_count ) );

        ImGuiSameLine();
        ImGuiText( va( "world entity count %i", loadedModel->mWorld.mEntities.size() ) );
      }
      else if( AssetViewImportingModel* loadingModel = GetLoadingModel( path ) )
      {
        ImGuiSameLine();
        ImGuiButton( "Loading" + LoadElipses() );
        if( loadingModel->mJob.GetStatus() == JobState::ThreadFinished )
        {
          if( !loadingModel->mJob.mErrors )
          {
            delete loadingModel;
            sLoadingModels.erase( sLoadingModels.find( path ) );


            loadedModel = new AssetViewImportedModel;
            Errors errors;
            loadedModel->mLoadInfo.Init( path.c_str(), errors );
            sLoadedModels[ path ] = loadedModel;

            EditorLoadInfo* loadInfo = &loadedModel->mLoadInfo;

            Entity* entityRoot = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
            entityRoot->mName = SplitFilepath( path ).mFilename;

            Vector< Entity* > entityNodes;
            for( int i = 0; i < loadInfo->parsedData->nodes_count; ++i )
            {
              Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
              entityNodes.push_back( entityNode );
            }


            for( int i = 0; i < loadInfo->parsedData->nodes_count; ++i )
            {
              const cgltf_node& node = loadInfo->parsedData->nodes[ i ];


              Entity* entityNode = loadedModel->mWorld.SpawnEntity( loadedModel->mEntityUUIDCounter.AllocateNewUUID() );
              entityNode->mName = node.name ? node.name : va( "node %i", i );
              entityNode->mRelativeSpace = DecomposeGLTFTransform( &node );

              if( node.mesh )
              {
                node.mesh->name;
                node.mesh->primitives;
                node.mesh->primitives_count;

                auto model = ( Model* )entityNode->AddNewComponent( Model().GetEntry() );
                model->mModelPath = path;
                //model->mModelName = node.mesh->name;
                model->mModelIndex = ( int )( node.mesh - loadInfo->parsedData->meshes );
                model->mTryingNewThing = true;


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

            if( false )
            {
              int w = 256;
              int h = 256;
              Render::TexSpec texSpecColor;
              texSpecColor.mImage.mFormat.mElementCount = 4;
              texSpecColor.mImage.mFormat.mPerElementByteCount = 1;
              texSpecColor.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
              texSpecColor.mImage.mWidth = w;
              texSpecColor.mImage.mHeight = h;
              texSpecColor.mBinding = ( Render::Binding )( ( int )Render::Binding::ShaderResource | ( int )Render::Binding::RenderTarget );
              Render::TextureHandle textureHandleColor = Render::CreateTexture( texSpecColor, TAC_STACK_FRAME );

              Render::TexSpec texSpecDepth;
              texSpecColor.mImage.mFormat.mElementCount = 1;
              texSpecColor.mImage.mFormat.mPerElementByteCount = 16;
              texSpecColor.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
              texSpecColor.mImage.mWidth = w;
              texSpecColor.mImage.mHeight = h;
              Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );

              Render::FramebufferHandle framebufferHandle = Render::CreateFramebufferForRenderToTexture(
                { textureHandleColor, textureHandleDepth }, TAC_STACK_FRAME );
              Render::ViewHandle viewHandle = Render::CreateView();
              Render::SetViewFramebuffer( viewHandle, framebufferHandle );
              Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
              Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );

              Camera camera;
              gCreation.mGamePresentation->RenderGameWorldToDesktopView( &loadedModel->mWorld, &camera, w, h, viewHandle );
            }
          }
        }
      }
      else
      {
        auto loading = TAC_NEW AssetViewImportingModel;
        loading->mJob.mPath = path;
        JobQueuePush( &loading->mJob );
        sLoadingModels[ path ] = loading;

        //Errors errors;
        //EditorLoadInfo editorLoadInfo = LoadEditorModelFileInfo( path, errors );

        //IEditorLoadInfo* loadInfo = EditorLoadInfoAllocate();
        //EditorLoadInfoFree( loadInfo );
      }
    }

    for( const String& path : otherPaths )
      ImGuiText( SplitFilepath( path ).mFilename );
  }

  void CreationUpdateAssetView()
  {
    ImGuiSetNextWindowStretch();
    const bool open = ImGuiBegin( "Asset View" );
    if( open )
    {
      ImGuiText( "--- Asset View ---" );
      if( sAssetViewErrors )
        ImGuiText( sAssetViewErrors.ToString() );

      const int oldStackSize = sAssetViewFolderStack.size();

      if( sAssetViewFolderStack.empty() )
      {
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

