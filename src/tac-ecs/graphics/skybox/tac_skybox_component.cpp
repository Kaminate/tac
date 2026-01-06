#include "tac_skybox_component.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/graphics/tac_graphics.h"

namespace Tac
{
  static auto CreateSkyboxComponent( World* world ) -> Component*
  {
    return Graphics::From( world )->CreateSkyboxComponent();
  }

  static void DestroySkyboxComponent( World* world, Component* component )
  {
    Graphics::From( world )->DestroySkyboxComponent( ( Skybox* )component );
  }

	static ComponentInfo* sComponentInfo;

	auto Skybox::GetEntry() const -> const ComponentInfo* { return sComponentInfo; }

	auto Skybox::GetSkybox( const Entity* entity ) -> const Skybox*
	{
		return ( Skybox* )entity->GetComponent( sComponentInfo );
	}

	auto Skybox::GetSkybox( dynmc Entity* entity ) -> dynmc Skybox*
	{
		return ( Skybox* )entity->GetComponent( sComponentInfo );
	}

	void Skybox::RegisterComponent()
	{
    *( sComponentInfo = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName       { "Skybox" },
      //sRegistryEntry->mNetVars = ComponentSkyboxBits;
      .mCreateFn   { CreateSkyboxComponent },
      .mDestroyFn  { DestroySkyboxComponent },
      //sRegistryEntry->mDebugImguiFn = SkyboxDebugImgui;
      //sRegistryEntry->mSaveFn = SaveSkyboxComponent;
      //sRegistryEntry->mLoadFn = LoadSkyboxComponent;
    };
	}

} // namespace Tac

