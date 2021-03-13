#include "src/creation/tacCreationAssetView.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacString.h"
#include "src/common/tacUtility.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"
#include "src/common/tacErrorHandling.h"


namespace Tac
{
  static String           sAssetViewFolderCur;
  static Vector< String > sAssetViewFolderStack;
  static Errors           sAssetViewErrors;
  static Vector< String > sAssetViewFiles;
  static Vector< String > sAssetViewFolders;

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
  static void UIFiles()
  {
    if( sAssetViewFiles.empty() )
      ImGuiText( "no files :(" );

    Vector< String > imagePaths;
    Vector< String > nonImagePaths;

    for( const String& path : sAssetViewFiles )
    {
      const bool isImage = 
        StringView( path ).ends_with( ".png" ) ||
        StringView( path ).ends_with( ".jpg" ) ||
        StringView( path ).ends_with( ".bmp" ) ||
        false;
      (isImage ? imagePaths : nonImagePaths).push_back( path );
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
    for( const String& path : nonImagePaths )
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

