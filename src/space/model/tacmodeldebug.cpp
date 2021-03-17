#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacShellTimer.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/tacOS.h"
#include "src/common/tacUtility.h"
#include "src/space/model/tacModel.h"

namespace Tac
{
  static bool IsModelPath( StringView s )
  {
    const char* modelExtensions[] =
    {
      ".glb",
      ".gltf",
    };
    for( const char* modelExtension : modelExtensions )
      if( s.ends_with( modelExtension ) )
        return true;
    return false;
  }

  void ModelDebugImguiChangeModel( Model* model )
  {
    if( ImGuiCollapsingHeader( "Change model" ) || model->mModelPath.empty() )
    {
      ImGuiIndent();
      TAC_ON_DESTRUCT( ImGuiUnindent() );

      static Vector< String > modelPaths;
      static Errors getfilesErrors;

      if( getfilesErrors )
        ImGuiText( getfilesErrors.ToString() );

      static bool needsRefresh = true;
      static double refreshSecTimestamp;
      const double curSecTimestamp = ShellGetElapsedSeconds();
      if( needsRefresh || ImGuiButton( "Refresh Model List" ) )
      {
        getfilesErrors.clear();
        needsRefresh = false;
        modelPaths.clear();
        Vector< String > allfiles;
        OS::GetFilesInDirectory( allfiles, "assets", OS::GetFilesInDirectoryFlags::Recursive, getfilesErrors );
        for( String file : allfiles )
          if( IsModelPath( file ) )
            modelPaths.push_back( file );

        refreshSecTimestamp = curSecTimestamp;
      }

      const double populateDuration = 0.1;
      const int numberOfFilesPopulate = int( ( curSecTimestamp - refreshSecTimestamp ) / populateDuration * modelPaths.size() );
      for( int i = 0; i < Min( ( int )modelPaths.size(), numberOfFilesPopulate ); ++i )
      {
        const String& filepath = modelPaths[ i ];
        if( ImGuiButton( filepath ) )
        {
          model->mModelPath = filepath;
          model->mesh = nullptr;
        }
      }
    }

    ImGuiInputText( "Model", model->mModelPath );


    //auto assetManager = mEntity->mWorld->mGameInterface->mAssetManager;
    //if( mGeometryUUID == NullGeometryUUID )
    //{
    //  ImGui::Text( "Geometry: none" );
    //}
    //else
    //{

    //  auto stuff = ( Stuff* )mEntity->GetComponent( ComponentRegistryEntryIndex::Stuff );

    //  ImGui::Text( "Model path: %s", assetManager->GetGeometryPath( mGeometryUUID ).c_str() );
    //  auto geometry = assetManager->GetGeometry( mGeometryUUID );

    //  if( geometry )
    //  {
    //    std::vector< ImTextureID > imguiTextureIDs;
    //    for( auto& subGeometry : geometry->mSubGeometries )
    //    {
    //      auto texture = assetManager->GetTexture( subGeometry.mTextureUUID );
    //      if( !texture )
    //        continue;
    //      imguiTextureIDs.push_back( ( ImTextureID )texture );
    //    }
    //    for( u32 i = 0; i < imguiTextureIDs.size(); ++i )
    //    {
    //      ImGui::Image( imguiTextureIDs[ i ], ImVec2( 50, 50 ) );
    //      if( i != imguiTextureIDs.size() - 1 && ( i + 1 ) % 10 )
    //        ImGui::SameLine();
    //    }

    //    Sphere sphere = geometry->mSphere;
    //    sphere.mCenter += stuff->mPosition;

    //    ImGui::DragFloat3( "Sphere center", &sphere.mCenter[ 0 ] );
    //    ImGui::DragFloat( "Sphere radius", &sphere.mRadius );
    //  }
    //}

    //auto buttonName = "Change Geometry";
    //auto popupName = "Choose Geometry";
    //if( ImGui::Button( buttonName ) )
    //  ImGui::OpenPopup( popupName );
    //if( !ImGui::BeginPopup( popupName ) )
    //  return;
    //OnDestruct( ImGui::EndPopup() );

    //for( auto pair : assetManager->GetGeometryMap() )
    //{
    //  auto path = pair.first;
    //  auto geometryUUID = pair.second;
    //  if( ImGui::Button( path.c_str() ) )
    //    mGeometryUUID = geometryUUID;
    //}
  }

  void ModelDebugImguiChangeTexture( Model* model )
  {
    TAC_UNUSED_PARAMETER( model );
    //auto assetManager = mEntity->mWorld->mGameInterface->mAssetManager;
    //if( mTextureUUID == NullTextureUUID )
    //{
    //  ImGui::Text( "Texture: none" );
    //}
    //else
    //{
    //  auto texture = assetManager->GetTexture( mTextureUUID );
    //  if( texture )
    //  {
    //    auto user_texture_id = ( ImTextureID )texture;
    //    ImGui::Image( user_texture_id, ImVec2( 50, 50 ) );
    //    ImGui::SameLine();
    //  }
    //  ImGui::Text( "Texture path: %s", assetManager->GetTexturePath( mTextureUUID ).c_str() );
    //  if( ImGui::Button( "Remove Texture" ) )
    //    mTextureUUID = NullTextureUUID;
    //}

    //const char* popupName = "Choose Texture";
    //if( ImGui::Button( "Change Texture" ) )
    //  ImGui::OpenPopup( popupName );
    //if( !ImGui::BeginPopup( popupName ) )
    //  return;
    //OnDestruct( ImGui::EndPopup() );
    //for( auto pair : assetManager->GetTextureMap() )
    //{
    //  auto path = pair.first;
    //  auto textureUUID = pair.second;
    //  auto texture = assetManager->GetTexture( path );
    //  if( !texture )
    //    continue;
    //  auto user_texture_id = ( ImTextureID )texture;
    //  auto pressed = ImGui::ImageButton( user_texture_id, ImVec2( 50, 50 ) );
    //  ImGui::SameLine();
    //  ImGui::Text( path.c_str() );
    //  if( !pressed )
    //    continue;
    //  mTextureUUID = textureUUID;
    //}
  }

  void ModelDebugImgui( Model* model )
  {
    if( ImGuiCollapsingHeader( "Mesh" ) && model->mesh )
    {
      Mesh* mesh = model->mesh;
      ImGuiText( va( "%.1f %.1f %.1f %.1f",
                 mesh->mTransform.m00,
                 mesh->mTransform.m01,
                 mesh->mTransform.m02,
                 mesh->mTransform.m03 ) );
      ImGuiText( va( "%.1f %.1f %.1f %.1f",
                 mesh->mTransform.m10,
                 mesh->mTransform.m11,
                 mesh->mTransform.m12,
                 mesh->mTransform.m13 ) );
      ImGuiText( va( "%.1f %.1f %.1f %.1f",
                 mesh->mTransform.m20,
                 mesh->mTransform.m21,
                 mesh->mTransform.m22,
                 mesh->mTransform.m23 ) );
      ImGuiText( va( "%.1f %.1f %.1f %.1f",
                 mesh->mTransform.m30,
                 mesh->mTransform.m31,
                 mesh->mTransform.m32,
                 mesh->mTransform.m33 ) );
      static int iSelectedSubmesh = -1;
      for( SubMesh& subMesh : mesh->mSubMeshes )
      {
        const int iSubMesh = ( int )( &subMesh - mesh->mSubMeshes.data() );
        if( ImGuiSelectable(
          va( "submesh %i: %s", iSubMesh, subMesh.mName.c_str() ),
          iSubMesh == iSelectedSubmesh ) )
          iSelectedSubmesh = iSubMesh;
      }
      if( ( unsigned )iSelectedSubmesh < ( unsigned )mesh->mSubMeshes.size() )
      {
        ImGuiIndent();
        SubMesh& subMesh = mesh->mSubMeshes[ iSelectedSubmesh ];
        ImGuiText( subMesh.mName );
        ImGuiText( va( "tri count: %i", subMesh.mTris.size() ) );
        ImGuiUnindent();
      }
    }
    ModelDebugImguiChangeModel( model );
    ModelDebugImguiChangeTexture( model );
    ImGuiDragFloat( "r", &model->mColorRGB[ 0 ] );
    ImGuiDragFloat( "g", &model->mColorRGB[ 1 ] );
    ImGuiDragFloat( "b", &model->mColorRGB[ 2 ] );
  }

  //void ModelDebugImgui( Component* component ) { ModelDebugImgui( ( Model* )component ); }

}

