#include "tac_material_input_layout.h" // self-inc
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{

  static const MetaType* FindMetaType( StringView str )
  {
    const Array allMetaTypes
    {
      &GetMetaType<v2>(),
      &GetMetaType<v3>(),
      &GetMetaType<v4>(),
      &GetMetaType<float>(),
      &GetMetaType<u32>(),
    };

    for( const MetaType* metaType : allMetaTypes )
      if( ( StringView )metaType->GetName() == str )
        return metaType;

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  MaterialInputLayout MaterialInputLayout::FromJson( const Json* inputLayout )
  {
    Vector< Variable > vars;
    if( inputLayout )
    {
      for( Json* varJson : inputLayout->mArrayElements )
      {
        const String typeStr{ varJson->GetChild( "type" ).mString };

        MaterialInputLayout::Variable var;
        var.mMetaType = FindMetaType( typeStr );
        var.mName = varJson->GetChild( "name" ).mString;
        var.mSemantic = varJson->GetChild( "semantic" ).mString;

        vars.push_back( var );
      }
    }

    MaterialInputLayout vso;
    vso.mVariables = vars;
    return vso;
  }

  Json                MaterialInputLayout::ToJson( const MaterialInputLayout& vso )
  {
    Json inputLayout;
    for( const MaterialInputLayout::Variable& var : vso.mVariables )
    {
      Json& varJson{ *inputLayout.AddChild() };
      varJson[ "name" ] = ( StringView )var.mName;
      varJson[ "semantic" ] = ( StringView )var.mSemantic;
      varJson[ "type" ] = ( StringView )var.mMetaType->GetName();
    }
    return inputLayout;
  }
} // namespace Tac

