#include "tac_material.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"

namespace Tac
{
  static ComponentRegistryEntry* sComponentRegistryEntry;

  static Component* CreateMaterialComponent( World* world )
  {
    return GetGraphics( world )->CreateMaterialComponent();
  }

  static void       DestroyMaterialComponent( World* world, Component* component )
  {
    GetGraphics( world )->DestroyMaterialComponent( ( Material* )component );
  }

  static void       SaveMaterialComponent( Json& materialJson, Component* component )
  {
    auto material { ( Material* )component };
  }

  static void       LoadMaterialComponent( Json& materialJson, Component* component )
  {
    auto material = ( Material* )component;

  }

  // ------------------------

  Material*                        Material::GetMaterial( Entity* entity )
  {
    return ( Material* )entity->GetComponent( sComponentRegistryEntry );
  }

  const Material*                  Material::GetMaterial( const Entity* entity )
  {
    return ( Material* )entity->GetComponent( sComponentRegistryEntry );
  }

  const ComponentRegistryEntry*    Material::GetEntry() const
  {
    return sComponentRegistryEntry;
  }

  void                             Material::RegisterComponent()
  {
    sComponentRegistryEntry = ComponentRegistry_RegisterComponent();
    sComponentRegistryEntry->mName = "Material";
    //sComponentRegistryEntry->mNetVars = ComponentMaterialBits;
    sComponentRegistryEntry->mCreateFn = CreateMaterialComponent;
    sComponentRegistryEntry->mDestroyFn = DestroyMaterialComponent;
    sComponentRegistryEntry->mDebugImguiFn = []( Component* component )
      {
        Material::DebugImgui( ( Material* )component );
      };
    sComponentRegistryEntry->mSaveFn = SaveMaterialComponent;
    sComponentRegistryEntry->mLoadFn = LoadMaterialComponent;
  }

  void                             Material::DebugImgui(Material* material)
  {
  }


} // namespace Tac

