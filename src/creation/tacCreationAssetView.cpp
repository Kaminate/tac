#include "src/creation/tacCreationAssetView.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacString.h"
#include "src/common/tacUtility.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"
#include "src/common/tacErrorHandling.h"
#include "src/space/tacworld.h"

#include <map>

namespace Tac
{
  static String           sAssetViewFolderCur;
  static Vector< String > sAssetViewFolderStack;
  static Errors           sAssetViewErrors;
  static Vector< String > sAssetViewFiles;
  static Vector< String > sAssetViewFolders;

  static World            sAssetViewWorld;

  static std::map< String, AssetViewImportedModel* >  sLoadedModels;
  static std::map< String, AssetViewImportingModel* >  sLoadingModels;

  struct AssetViewImportingModel
  {

  };

  struct AssetViewImportedModel
  {
    // somethings are embedded in the model file
    // - mesh / material
    // - possible textures
    // other things are stored in external files
    // - textures
  };

  static void PopulateFolderContents()
  {
    sAssetViewFiles.clear();
    sAssetViewFolders.clear();
    OS::GetFilesInDirectory( sAssetViewFiles, sAssetViewFolderCur, OS::GetFilesInDirectoryFlags::Default, sAssetViewErrors );
    OS::GetDirectoriesInDirectory( sAssetViewFolders, sAssetViewFolderCur, sAssetViewErrors );
  }

  static void UICurrAndPrevFolders()
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

  static void UINextFolders()
  {
    for( const String& path : sAssetViewFolders )
    {
      const int iPath = &path - sAssetViewFolders.data();
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
      const int iPath = &path - imagePaths.data();
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
      if( iPath % 3 < 3 - 1 && iPath!= imagePaths.size() - 1 )
        ImGuiSameLine();
    }

    for( const String& path : modelPaths )
    {
      const String filename = SplitFilepath( path ).mFilename;
      const String filenamePrefab = StripExt( filename ) + ".prefab";
      ImGuiText( filename );
      
      bool importFound = false;
      for( const String& assetViewFile : sAssetViewFiles )
        if( SplitFilepath( assetViewFile ).mFilename == filenamePrefab )
          importFound = true;
      if( !importFound )
      {
        ImGuiSameLine();
        ImGuiButton( "Import" );
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

      UICurrAndPrevFolders();
      UINextFolders();
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

