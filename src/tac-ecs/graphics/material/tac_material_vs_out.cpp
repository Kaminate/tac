#include "tac_material_vs_out.h" // self-inc
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

  MaterialVSOut MaterialVSOut::FromJson( const Json* json )
  {
    Vector< Variable > vars;
    if( json )
    {
      for( Json* varJson : json->mArrayElements )
      {
        const String typeStr{ varJson->GetChild( "type" ).mString };

        MaterialVSOut::Variable var;
        var.mMetaType = FindMetaType( typeStr );
        var.mName = varJson->GetChild( "name" ).mString;
        var.mSemantic = varJson->GetChild( "semantic" ).mString;

        vars.push_back( var );
      }
    }

    MaterialVSOut vso;
    vso.mVariables = vars;
    return vso;
  }

  Json          MaterialVSOut::ToJson( const MaterialVSOut& vso )
  {
    Json json;
    for( const MaterialVSOut::Variable& var : vso.mVariables )
    {
      Json& varJson{ *json.AddChild() };
      varJson[ "name" ] = ( StringView )var.mName;
      varJson[ "semantic" ] = ( StringView )var.mSemantic;
      varJson[ "type" ] = ( StringView )var.mMetaType->GetName();
    }

    return json;
  }
} // namespace Tac

