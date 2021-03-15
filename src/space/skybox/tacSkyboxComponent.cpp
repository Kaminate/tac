#include "src/space/skybox/tacSkyboxComponent.h"
#include "src/space/tacentity.h"
#include "src/space/graphics/tacgraphics.h"

namespace Tac
{
	static ComponentRegistryEntry* sRegistryEntry;

	const Skybox*                 Skybox::GetSkybox( const Entity* entity )
	{
		return ( Skybox* )entity->GetComponent( sRegistryEntry );
	}

	Skybox*                       Skybox::GetSkybox( Entity* entity )
	{
		return ( Skybox* )entity->GetComponent( sRegistryEntry );
	}

	const ComponentRegistryEntry* Skybox::GetEntry() const
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

	void              RegisterSkyboxComponent()
	{
    sRegistryEntry = ComponentRegistry_RegisterComponent();
		sRegistryEntry->mName = "Skybox";
		//sRegistryEntry->mNetworkBits = ComponentSkyboxBits;
		sRegistryEntry->mCreateFn = CreateSkyboxComponent;
		sRegistryEntry->mDestroyFn = DestroySkyboxComponent;
		//sRegistryEntry->mDebugImguiFn = SkyboxDebugImgui;
		//sRegistryEntry->mSaveFn = SaveSkyboxComponent;
		//sRegistryEntry->mLoadFn = LoadSkyboxComponent;
	}

}

