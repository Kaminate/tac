#include "tac_skybox_component.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/graphics/tac_graphics.h"

namespace Tac
{
	static ComponentInfo* sRegistryEntry;

	const Skybox*                 Skybox::GetSkybox( const Entity* entity )
	{
		return ( Skybox* )entity->GetComponent( sRegistryEntry );
	}

	Skybox*                       Skybox::GetSkybox( Entity* entity )
	{
		return ( Skybox* )entity->GetComponent( sRegistryEntry );
	}

	const ComponentInfo* Skybox::GetEntry() const
	{
		return sRegistryEntry;
  }

  static Component* CreateSkyboxComponent( World* world )
  {
    return  GetGraphics( world )->CreateSkyboxComponent();
  }

  static void       DestroySkyboxComponent( World* world, Component* component )
  {
    GetGraphics( world )->DestroySkyboxComponent( ( Skybox* )component );
  }

	void              Skybox::RegisterComponent()
	{
    *( sRegistryEntry = ComponentInfo::Register() ) = ComponentInfo
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

