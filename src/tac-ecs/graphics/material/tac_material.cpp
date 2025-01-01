#include "tac_material.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell.h" // AssetOpenDialog
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_meta.h"

namespace Tac
{
  TAC_META_REGISTER_COMPOSITE_BEGIN( Material );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mIsGlTF_PBR_MetallicRoughness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mIsGlTF_PBR_SpecularGlossiness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mPBR_Factor_Metallic );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mPBR_Factor_Roughness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mPBR_Factor_Diffuse );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mPBR_Factor_Specular );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mPBR_Factor_Glossiness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mColor );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mEmissive );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mTextureDiffuse );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mTextureSpecular );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mTextureGlossiness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mTextureMetallic );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mTextureRoughness );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mShaderGraph );
  TAC_META_REGISTER_COMPOSITE_MEMBER( mRenderEnabled );
  TAC_META_REGISTER_COMPOSITE_END( Material );


  static ComponentInfo* sComponentInfo;

  static Component* CreateMaterialComponent( World* world )
  {
    return Graphics::From( world )->CreateMaterialComponent();
  }

  static void       DestroyMaterialComponent( World* world, Component* component )
  {
    Graphics::From( world )->DestroyMaterialComponent( ( Material* )component );
  }

#if 0
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

    TAC_ASSERT_UNIMPLEMENTED;

    //const MetaType& metaMaterial{ GetMetaType< Material >() };
    //metaMaterial.JsonSerialize( &json, component );

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
#endif

  struct AssetPathUIHelper
  {
    AssetPathUIHelper( StringView name ) : mName{ name } {}
    void DebugImgui( AssetPathString& assetPathString )
    {
      Errors& errors{ mErrors };
      const StringView name{ mName };
      const StringView displayStr{ assetPathString.empty() ? ( StringView )"<none>"
                                                           : ( StringView )assetPathString };
      ImGuiText( ShortFixedString::Concat( name, ": ", displayStr ) );
      if( ImGuiButton( ShortFixedString::Concat( "Select ", name ) ) )
      {
        errors.clear();
        if( const AssetPathString path{ AssetOpenDialog( errors ) }; !errors && !path.empty() )
          assetPathString = path;
      }

      if( errors )
        ImGuiText( errors.ToString() );
    }

    StringView mName;
    Errors     mErrors;
  };

  struct BindingsUI
  {
    void RefreshBindings( Material* material, Errors& errors )
    {
#if 0 // commented out after introduction of shader graph
      if( !material->mMaterialShader.empty() )
      {
        mBindings = {};
        return;
      }

      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };


      const Render::ProgramParams programParams
      {
        .mInputs{ material->mMaterialShader },
      };

      const Render::ProgramHandle programHandle{
        renderDevice->CreateProgram( programParams, errors ) };

      if( !errors )
      {
        mBindings = renderDevice->GetProgramBindings_TEST( programHandle );
        renderDevice->DestroyProgram( programHandle );
      }

#endif
    }

    void UI( Material* material )
    {
      Errors& errors{ mErrors };
      if( ImGuiButton( "Refresh bindings" ) )
      {
        errors.clear();
        RefreshBindings( material, errors );
      }

      ImGuiText( "Bindings: " + ( mBindings.empty() ? "<none>" : mBindings ) );
      if( errors )
        ImGuiText( errors.ToString() );
    }

    Errors mErrors;
    String mBindings;
  };


  // -----------------------------------------------------------------------------------------------

  dynmc Material*                  Material::GetMaterial( dynmc Entity* entity )
  {
    return ( Material* )entity->GetComponent( sComponentInfo );
  }

  const Material*                  Material::GetMaterial( const Entity* entity )
  {
    return ( Material* )entity->GetComponent( sComponentInfo );
  }

  const ComponentInfo*             Material::GetEntry() const { return sComponentInfo; }

  void                             Material::RegisterComponent()
  {
    const MetaCompositeType* metaType{ ( MetaCompositeType* )&GetMetaType< Material >() };

    NetVarRegistration netVarRegistration {};
    netVarRegistration.mNetMembers.SetAll();
    netVarRegistration.mMetaType = metaType;

    *( sComponentInfo = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName               { "Material" },
      .mCreateFn           { CreateMaterialComponent },
      .mDestroyFn          { DestroyMaterialComponent },
      .mDebugImguiFn       { []( Component* c ) { Material::DebugImgui( ( Material* )c ); } },
      .mNetVarRegistration { netVarRegistration },
      .mMetaType           { metaType },
    };
  }

  void                             Material::DebugImgui( Material* material )
  {
    static AssetPathUIHelper ui_shaderGraph( "Shader Graph" );
    static AssetPathUIHelper ui_pbrTexture_Metallic( "pbr metallic texture" );
    static AssetPathUIHelper ui_pbrTexture_Roughness( "pbr roughness texture" );
    static AssetPathUIHelper ui_pbrTexture_Diffuse( "pbr diffuse texture" );
    static AssetPathUIHelper ui_pbrTexture_Specular( "pbr specular texture" );
    static AssetPathUIHelper ui_pbrTexture_Glossiness( "pbr glossiness texture" );
    static BindingsUI sBindingsUI;

    ImGuiCheckbox( "Enabled", &material->mRenderEnabled );

    ui_shaderGraph.DebugImgui( material->mShaderGraph );

    sBindingsUI.UI( material );

    if( ImGuiCheckbox( "gltf pbr mr", &material->mIsGlTF_PBR_MetallicRoughness )
        && material->mIsGlTF_PBR_MetallicRoughness )
      material->mIsGlTF_PBR_SpecularGlossiness = false;

    if( ImGuiCheckbox( "gltf pbr sg", &material->mIsGlTF_PBR_SpecularGlossiness )
        && material->mIsGlTF_PBR_SpecularGlossiness )
      material->mIsGlTF_PBR_MetallicRoughness = false;

    if( material->mIsGlTF_PBR_MetallicRoughness )
    {
      ImGuiDragFloat( "gltf pbr metallic factor", &material->mPBR_Factor_Metallic );
      ImGuiDragFloat( "gltf pbr roughness factor", &material->mPBR_Factor_Roughness );
      ui_pbrTexture_Metallic.DebugImgui( material->mTextureMetallic );
      ui_pbrTexture_Roughness.DebugImgui( material->mTextureRoughness );
    }

    if( material->mIsGlTF_PBR_SpecularGlossiness )
    {
      ImGuiDragFloat3( "gltf pbr diffuse factor", material->mPBR_Factor_Diffuse.data() );
      ImGuiDragFloat3( "gltf pbr specular factor", material->mPBR_Factor_Specular.data() );
      ImGuiDragFloat( "gltf pbr glossiness factor", &material->mPBR_Factor_Glossiness );
      ui_pbrTexture_Diffuse.DebugImgui( material->mTextureDiffuse );
      ui_pbrTexture_Specular.DebugImgui( material->mTextureSpecular );
      ui_pbrTexture_Glossiness.DebugImgui( material->mTextureGlossiness );
    }

    ImGuiDragFloat4( "color", material->mColor.data() );
    ImGuiDragFloat3( "emissive", material->mEmissive.data() );
  }


} // namespace Tac

