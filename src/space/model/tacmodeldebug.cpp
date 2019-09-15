#include "common/graphics/tacImGui.h"
#include "common/tacPreprocessor.h"
#include "common/tacErrorHandling.h"
#include "common/tacOS.h"
#include "common/tacUtility.h"
#include "space/model/tacmodel.h"


void TacModelDebugImguiChangeModel( TacModel* model )
{
  if( TacImGuiCollapsingHeader( "Change model" ) )
  {
    TacImGuiIndent();
    OnDestruct( TacImGuiUnindent() );
    static bool firsttime;
    static TacVector< TacString > gltf_filepaths;
    static TacErrors getfilesErrors;
    if( !firsttime )
    {
      firsttime = true;
      TacVector< TacString > allfiles;
      TacOS::Instance->GetDirFilesRecursive( allfiles, "assets", getfilesErrors );
      for( TacString file : allfiles )
        if( TacEndsWith( TacToLower( file ), ".gltf" ) )
          gltf_filepaths.push_back( file );
    }

    for( TacString filepath : gltf_filepaths )
    {
      if( TacImGuiButton( filepath ) )
      {
        model->mGLTFPath = filepath;
        model->mesh = nullptr;
      }
    }
  }
  TacImGuiInputText( "Model", model->mGLTFPath );


  //auto assetManager = mEntity->mWorld->mGameInterface->mAssetManager;
  //if( mGeometryUUID == TacNullGeometryUUID )
  //{
  //  ImGui::Text( "Geometry: none" );
  //}
  //else
  //{

  //  auto stuff = ( TacStuff* )mEntity->GetComponent( TacComponentRegistryEntryIndex::Stuff );

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

  //    TacSphere sphere = geometry->mSphere;
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

void TacModelDebugImguiChangeTexture( TacModel* model )
{
  //auto assetManager = mEntity->mWorld->mGameInterface->mAssetManager;
  //if( mTextureUUID == TacNullTextureUUID )
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
  //    mTextureUUID = TacNullTextureUUID;
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

void TacModelDebugImgui( TacModel* model )
{
  TacModelDebugImguiChangeModel( model );
  TacModelDebugImguiChangeTexture( model );
  TacImGuiDragFloat( "r", &model->mColorRGB[ 0 ] );
  TacImGuiDragFloat( "g", &model->mColorRGB[ 1 ] );
  TacImGuiDragFloat( "b", &model->mColorRGB[ 2 ] );
}

void TacModelDebugImgui( TacComponent* component )
{
  TacModelDebugImgui( ( TacModel* )component );
}
