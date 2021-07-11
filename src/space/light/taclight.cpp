#include "src/space/light/tacLight.h"
#include "src/space/tacentity.h"
#include "src/space/graphics/tacgraphics.h"

namespace Tac
{
  static ComponentRegistryEntry* sComponentRegistryEntry;

	static Component* CreateLightComponent( World* world )
	{
		return GetGraphics( world )->CreateLightComponent();
	}

	static void       DestroyLightComponent( World* world, Component* component )
	{
		GetGraphics( world )->DestroyLightComponent( ( Light* )component );
	}

	static void       SaveLightComponent( Json& lightJson, Component* component )
	{
		auto light = ( Light* )component;
	}

	static void       LoadLightComponent( Json& lightJson, Component* component )
	{
		auto light = ( Light* )component;
	}


  Light*                        Light::GetLight( Entity* entity )
  {
    return ( Light* )entity->GetComponent( sComponentRegistryEntry );
  }

  const Light*                  Light::GetLight( const Entity* entity )
  {
    return ( Light* )entity->GetComponent( sComponentRegistryEntry );
  }

  const ComponentRegistryEntry* Light::GetEntry() const
  {
    return sComponentRegistryEntry;
  }

  void LightDebugImgui( Light* );

  void                                   RegisterLightComponent()
  {
    sComponentRegistryEntry = ComponentRegistry_RegisterComponent();
    sComponentRegistryEntry->mName = "Light";
    //sComponentRegistryEntry->mNetworkBits = ComponentLightBits;
    sComponentRegistryEntry->mCreateFn = CreateLightComponent;
    sComponentRegistryEntry->mDestroyFn = DestroyLightComponent;
    sComponentRegistryEntry->mDebugImguiFn = []( Component* component ){ LightDebugImgui( ( Light* )component ); };
    sComponentRegistryEntry->mSaveFn = SaveLightComponent;
    sComponentRegistryEntry->mLoadFn = LoadLightComponent;
  }


}

