#include "tac_camera_component.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
//#include "tac-ecs/graphics/model/tac_model_debug.h"
//#include "tac-engine-core/asset/tac_asset_hash_cache.h"
//#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-std-lib/dataprocess/tac_json.h"
//#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-std-lib/os/tac_os.h"
//#include "tac-std-lib/string/tac_string_util.h"
//#include "tac-std-lib/meta/tac_meta.h"
//#include "tac-std-lib/meta/tac_meta_composite.h"
//#include "tac-std-lib/string/tac_string_meta.h"

namespace Tac
{
  static ComponentInfo* sEntry;

  TAC_META_REGISTER_STRUCT_BEGIN( CameraComponent );
  TAC_META_REGISTER_STRUCT_END( CameraComponent );

	static auto CreateCameraComponent( World* world ) -> Component*
	{
		return Graphics::From( world )->CreateCameraComponent();
	}

	static void DestroyCameraComponent( World* world, Component* component )
	{
    Graphics::From( world )->DestroyCameraComponent( ( CameraComponent* )component );
	}

  static void CameraComponentDebugImgui( Component* component )
  {
    auto camera{ ( CameraComponent* )component };
    TAC_UNUSED_PARAMETER( camera );
  }


  // -----------------------------------------------------------------------------------------------

	auto CameraComponent::GetCamera( const Entity* entity ) -> const CameraComponent* { return ( CameraComponent* )entity->GetComponent( sEntry ); }

	auto CameraComponent::GetCamera( dynmc Entity* entity ) -> dynmc CameraComponent* { return ( CameraComponent* )entity->GetComponent( sEntry ); }

  auto CameraComponent::GetEntry() const -> const ComponentInfo* { return sEntry; }

	void CameraComponent::RegisterComponent()
	{
    const MetaCompositeType* metaType{ ( MetaCompositeType* )&GetMetaType< CameraComponent >() };

    NetMembers netMembers{};
    netMembers.SetAll();

    const NetVarRegistration netVarRegistration
    {
      .mNetMembers { netMembers },
      .mMetaType   { metaType },
    };

    * ( sEntry = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName               { "CameraComponent" },
      .mCreateFn           { CreateCameraComponent},
      .mDestroyFn          { DestroyCameraComponent },
      .mDebugImguiFn       { CameraComponentDebugImgui },
      .mNetVarRegistration { netVarRegistration },
      .mMetaType           { metaType },
    };
	}

} // namespace Tac

