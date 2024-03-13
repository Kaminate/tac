#include "tac-std-lib/assetmanagers/tac_mesh.h"
#include "tac-std-lib/assetmanagers/tac_model_asset_manager.h"
#include "tac-std-lib/shell/tac_shell.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/system/tac_filesystem.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/os/tac_os.h"
#include "space/graphics/model/tac_model.h"
#include "space/presentation/tac_game_presentation.h"

namespace Tac
{
  static bool IsModelPath( const AssetPathStringView& path )
  {
    const char* exts[] =
    {
      "glb",
      "gltf",
      "obj",
    };
    for( const char* ext : exts )
      if( path.ends_with( ext ) )
        return true;

    return false;
  }

  void ModelDebugImguiChangeModel( Model* model )
  {
    if( ImGuiCollapsingHeader( "Change model" ) || model->mModelPath.empty() )
    {
      ImGuiIndent();
      TAC_ON_DESTRUCT( ImGuiUnindent() );

      static AssetPathStrings modelPaths;
      static Errors getfilesErrors;

      if( getfilesErrors )
        ImGuiText( getfilesErrors.ToString() );

      static bool needsRefresh = true;
      static Timestamp refreshSecTimestamp;
      const Timestamp curSecTimestamp = Timestep::GetElapsedTime();
      if( needsRefresh || ImGuiButton( "Refresh Model List" ) )
      {
        getfilesErrors.clear();
        needsRefresh = false;
        modelPaths.clear();
        const AssetPathStrings allfiles = IterateAssetsInDir( "assets",
                                                              AssetIterateType::Recursive,
                                                              getfilesErrors );
        if( getfilesErrors )
          return;

        for( const AssetPathString& file : allfiles )
          if( IsModelPath( file  ) )
            modelPaths.push_back( file );

        refreshSecTimestamp = curSecTimestamp;
      }

      // how long (in seconds) it should take to populate the list
      const float populateDuration = 0.1f;

      const int numberOfFilesPopulate = int(
        ( curSecTimestamp - refreshSecTimestamp ) /
        populateDuration * modelPaths.size() );
      const int numberOfFilesToShow = Min( ( int )modelPaths.size(), numberOfFilesPopulate );

      for( int i = 0; i < numberOfFilesToShow; ++i )
      {
        const AssetPathStringView filepath = modelPaths[ i ];
        if( ImGuiButton( filepath ) )
        {
          model->mModelPath = filepath;
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
    //    Vector< ImTextureID > imguiTextureIDs;
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

  static void ModelDebugImguiMesh(  Model* model  )
  {
    if( !ImGuiCollapsingHeader( "Mesh" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    const Mesh* mesh = GamePresentationGetModelMesh( model );
    if( !mesh )
    {
      const String ellipses( "...", ( int )Timestep::GetElapsedTime() % 4 );
      ImGuiText( "Loading" + ellipses );
      return;
    }

    //ImGuiText( va( "%.1f %.1f %.1f %.1f",
    //               mesh->mTransform.m00,
    //               mesh->mTransform.m01,
    //               mesh->mTransform.m02,
    //               mesh->mTransform.m03 ) );
    //ImGuiText( va( "%.1f %.1f %.1f %.1f",
    //               mesh->mTransform.m10,
    //               mesh->mTransform.m11,
    //               mesh->mTransform.m12,
    //               mesh->mTransform.m13 ) );
    //ImGuiText( va( "%.1f %.1f %.1f %.1f",
    //               mesh->mTransform.m20,
    //               mesh->mTransform.m21,
    //               mesh->mTransform.m22,
    //               mesh->mTransform.m23 ) );
    //ImGuiText( va( "%.1f %.1f %.1f %.1f",
    //               mesh->mTransform.m30,
    //               mesh->mTransform.m31,
    //               mesh->mTransform.m32,
    //               mesh->mTransform.m33 ) );

    ImGuiText( ShortFixedString::Concat( "model index: ", ToString( model->mModelIndex ) ) );
    ImGuiDragInt( "model index", &model->mModelIndex );
    static int iSelectedSubmesh = -1;
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const int iSubMesh = ( int )( &subMesh - mesh->mSubMeshes.data() );
      const bool selected = iSubMesh == iSelectedSubmesh;

      const ShortFixedString str = ShortFixedString::Concat( "submesh ",
                                                             ToString( iSubMesh ),
                                                             ": ",
                                                             subMesh.mName );

      if( ImGuiSelectable( str, selected ) )
        iSelectedSubmesh = iSubMesh;
    }
    if( ( unsigned )iSelectedSubmesh < ( unsigned )mesh->mSubMeshes.size() )
    {
      const SubMesh& subMesh = mesh->mSubMeshes[ iSelectedSubmesh ];

      ImGuiIndent();
      ImGuiText( subMesh.mName );
      ImGuiText( ShortFixedString::Concat( "tri count: ", ToString( subMesh.mTris.size() ) ) );
      ImGuiUnindent();
    }
  }

  static void ModelDebugImguiColor( Model* model )
  {
    if( ImGuiDragFloat3( "rgb", model->mColorRGB.data() ) )
      for( float& f : model->mColorRGB )
        f = Saturate( f );
  }

  void ModelDebugImgui( Model* model )
  {
    ModelDebugImguiMesh( model );
    ModelDebugImguiChangeModel( model );
    ModelDebugImguiChangeTexture( model );
    ModelDebugImguiColor( model );
  }

  //void ModelDebugImgui( Component* component ) { ModelDebugImgui( ( Model* )component ); }

}

