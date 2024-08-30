#include "tac_material.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/string/tac_string_meta.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"

namespace Tac
{

  TAC_META_REGISTER_COMPOSITE_BEGIN( Material )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mIsGlTF_PBR_MetallicRoughness )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mIsGlTF_PBR_SpecularGlossiness )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mColor )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mEmissive )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mMaterialShader )
  TAC_META_REGISTER_COMPOSITE_MEMBER( Material, mRenderEnabled )
  TAC_META_REGISTER_COMPOSITE_END( Material );


  static ComponentRegistryEntry* sComponentRegistryEntry;

  static Component* CreateMaterialComponent( World* world )
  {
    return GetGraphics( world )->CreateMaterialComponent();
  }

  static void       DestroyMaterialComponent( World* world, Component* component )
  {
    GetGraphics( world )->DestroyMaterialComponent( ( Material* )component );
  }


  static void       SaveMaterialComponent( Json& json, Component* component )
  {
    Material* material{ ( Material* )component };

    json[ TAC_MEMBER_NAME( Material, mIsGlTF_PBR_MetallicRoughness ) ].SetBool( material->mIsGlTF_PBR_MetallicRoughness );
    json[ TAC_MEMBER_NAME( Material, mIsGlTF_PBR_SpecularGlossiness ) ].SetBool( material->mIsGlTF_PBR_SpecularGlossiness );
    json[ TAC_MEMBER_NAME( Material, mColor ) ][ 0 ].SetNumber( material->mColor[0]);
    json[ TAC_MEMBER_NAME( Material, mColor ) ][ 1 ].SetNumber( material->mColor[1]);
    json[ TAC_MEMBER_NAME( Material, mColor ) ][ 2 ].SetNumber( material->mColor[2]);
    json[ TAC_MEMBER_NAME( Material, mColor ) ][ 3 ].SetNumber( material->mColor[3]);
    json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 0 ].SetNumber( material->mEmissive[0]);
    json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 1 ].SetNumber( material->mEmissive[1]);
    json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 2 ].SetNumber( material->mEmissive[2] );
    json[ TAC_MEMBER_NAME( Material, mRenderEnabled ) ].SetBool( material->mRenderEnabled );
    json[ TAC_MEMBER_NAME( Material, mMaterialShader ) ].SetString( material->mMaterialShader );

    const MetaType& metaMaterial{ GetMetaType< Material >() };
    metaMaterial.JsonSerialize( &json, component );

  }

  static void       LoadMaterialComponent( Json& json, Component* component )
  {
    Material* material{ ( Material* )component };

    material->mIsGlTF_PBR_MetallicRoughness = json[ TAC_MEMBER_NAME( Material, mIsGlTF_PBR_MetallicRoughness ) ];
    material->mIsGlTF_PBR_SpecularGlossiness = json[ TAC_MEMBER_NAME( Material, mIsGlTF_PBR_SpecularGlossiness ) ];
    material->mColor[ 0 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mColor ) ][ 0 ];
    material->mColor[ 1 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mColor ) ][ 1 ];
    material->mColor[ 2 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mColor ) ][ 2 ];
    material->mColor[ 3 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mColor ) ][ 3 ];
    material->mEmissive[ 0 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 0 ];
    material->mEmissive[ 1 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 1 ];
    material->mEmissive[ 2 ] = ( float )( JsonNumber )json[ TAC_MEMBER_NAME( Material, mEmissive ) ][ 2 ];
    material->mRenderEnabled = json[ TAC_MEMBER_NAME( Material, mRenderEnabled ) ];
    material->mMaterialShader = json[ TAC_MEMBER_NAME( Material, mMaterialShader ) ];
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


    ImGuiCheckbox( "gltf pbr mr", &material->mIsGlTF_PBR_MetallicRoughness );
    ImGuiCheckbox( "gltf pbr sg", &material->mIsGlTF_PBR_SpecularGlossiness );
    ImGuiDragFloat4( "color", material->mColor.data() );
    ImGuiDragFloat3( "emissive", material->mEmissive.data() );
  }


} // namespace Tac

