#include "tac_sprite3d.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/model/tac_model_debug.h"
#include "tac-engine-core/asset/tac_asset_hash_cache.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/string/tac_string_meta.h"

namespace Tac
{
  static ComponentInfo* sEntry;

  TAC_META_REGISTER_STRUCT_BEGIN( Sprite3D );
  TAC_META_REGISTER_STRUCT_MEMBER( mTexturePath );
  TAC_META_REGISTER_STRUCT_END( Sprite3D );

  // -----------------------------------------------------------------------------------------------

  static void Sprite3DDebugImgui( Component* component )
  {
    Sprite3D* sprite{ ( Sprite3D* )component };
    ImGuiText( "Texture Path: %s", sprite->mTexturePath.empty() ? "<n/a>" : sprite->mTexturePath.c_str() );
    static Errors sImGuiErrors;
    if( ImGuiButton( "Change Texture" ) )
      if( const AssetPathStringView newPath{ AssetOpenDialog( sImGuiErrors ) }; !newPath.empty() )
        sprite->mTexturePath = newPath;
    if( sImGuiErrors )
      ImGuiText( sImGuiErrors.ToString() );
    if( !sprite->mTexturePath.empty() )
    {
      const Render::TextureHandle hTex{ TextureAssetManager::GetTexture( sprite->mTexturePath, sImGuiErrors ) };
      if( hTex.IsValid() )
      {
        const v3i texSize{ TextureAssetManager::GetTextureSize( sprite->mTexturePath, sImGuiErrors ) };
        const float aspect{texSize.x / (float)texSize.y};
        ImGuiImage( hTex.GetIndex(), v2( 100 * aspect, 100 ) );
      }
    }
  }

  auto Sprite3D::GetEntry() const -> const ComponentInfo* { return sEntry; }

	auto Sprite3D::GetSprite3D( const Entity* entity ) -> const Sprite3D* { return ( Sprite3D* )entity->GetComponent( sEntry ); }

	auto Sprite3D::GetSprite3D( dynmc Entity* entity ) -> dynmc Sprite3D* { return ( Sprite3D* )entity->GetComponent( sEntry ); }

	void Sprite3D::RegisterComponent()
	{
    const MetaCompositeType* metaType{ ( MetaCompositeType* )&GetMetaType< Sprite3D >() };
    const NetVarRegistration netVarRegistration {
      .mNetMembers { NetMembers::AllSet() },
      .mMetaType   { metaType },
    };
    const ComponentInfo::ComponentCreateFn createFn{
      []( World* world ) -> Component*
      {
        return Graphics::From( world )->CreateSprite3DComponent();
      }
    };
    const ComponentInfo::ComponentDestroyFn destroyFn{
      []( World* world, Component* component )
      {
        Graphics* graphics{ Graphics::From( world ) };
        graphics->DestroySprite3DComponent( ( Sprite3D* )component );
      }
    };
    * ( sEntry = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName               { "Sprite3D" },
      .mCreateFn           { createFn },
      .mDestroyFn          { destroyFn },
      .mDebugImguiFn       { Sprite3DDebugImgui },
      .mNetVarRegistration { netVarRegistration },
      .mMetaType           { metaType },
    };
	}

} // namespace Tac

