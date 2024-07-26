#include "tac_material.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-ecs/graphics/tac_graphics.h"
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
    Material* material{ ( Material* )component };
  }

  static void       LoadMaterialComponent( Json& materialJson, Component* component )
  {
    Material* material{ ( Material* )component };
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

  void                             Material::DebugImgui( Material* material )
  {
    static Errors errors;

    ImGuiCheckbox( "Enabled", &material->mRenderEnabled );
    if( !material->mMaterialShader.empty() )
      ImGuiText( "Shader: " + material->mMaterialShader );

    if( ImGuiButton( "Select Shader" ) )
    {
      errors.clear();
      const FileSys::Path shaderPath{ OS::OSOpenDialog( errors ) };
      if( shaderPath.has_stem() )
        material->mMaterialShader = shaderPath.stem().u8string();
    }

    if( errors )
      ImGuiText( errors.ToString() );

    static String bindings;
    if( !material->mMaterialShader.empty() && ImGuiButton( "Refresh bindings" ) )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

      const Render::ProgramParams programParams
      {
        .mFileStem{ material->mMaterialShader },
      };

      const Render::ProgramHandle programHandle{ renderDevice->CreateProgram( programParams, errors ) };
      if( !errors )
      {
        bindings = renderDevice->GetProgramBindings_TEST( programHandle );
        renderDevice->DestroyProgram( programHandle );
      }
    }

    if( bindings.empty() )
      ImGuiText( "Bindings: <none>" );
    else
      ImGuiText( "Bindings: " + bindings );
  }


} // namespace Tac

