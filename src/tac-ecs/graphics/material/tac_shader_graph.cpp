#include "tac_shader_graph.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  static const char* sJsonKeyInputLayout     { "input_layout" };
  static const char* sJsonKeyMaterialInputs  { "material_inputs" };
  static const char* sJsonKeyMaterialShader  { "material_shader" };

  // -----------------------------------------------------------------------------------------------

  Json        ShaderGraph::ToJson( const ShaderGraph& sg )
  {
    Json json;
    json[ sJsonKeyInputLayout ] = MaterialInputLayout::ToJson( sg.mMaterialInputLayout );
    json[ sJsonKeyMaterialInputs ] = MaterialInput::MaterialInput_to_Json( sg.mMaterialInputs );
    json[ sJsonKeyMaterialShader ] = ( StringView )sg.mMaterialShader;
    return json;
  }

  ShaderGraph ShaderGraph::FromJson( const Json& json )
  {
    const Json* materialShaderJson{ json.FindChild( sJsonKeyMaterialShader ) };
    const Json* inputLayoutJson{ json.FindChild( sJsonKeyInputLayout ) };
    const Json* materialInputsJson{ json.FindChild( sJsonKeyMaterialInputs ) };

    const MaterialInputLayout  materialInputLayout{
      MaterialInputLayout::FromJson( inputLayoutJson ) };

    const MaterialInput        materialInputs{
      MaterialInput::Json_to_MaterialInput( materialInputsJson ) };

    const String               materialShader{
      materialShaderJson ? materialShaderJson->mString : "" };

    return ShaderGraph
    {
      .mMaterialInputLayout { materialInputLayout },
      .mMaterialInputs      { materialInputs },
      .mMaterialShader      { materialShader },
    };
  }

  void        ShaderGraph::ToPath( const ShaderGraph& sg,
                                   AssetPathStringView path,
                                   Errors& errors )
  {
    TAC_ASSERT( !path.empty() );

    const Json json{ ShaderGraph::ToJson( sg ) };
    const String string{ json.Stringify() };

    TAC_CALL( SaveToFile( path, string.data(), string.size(), errors ) );
  }

  ShaderGraph ShaderGraph::FromPath( AssetPathStringView path, Errors& errors )
  {
    TAC_ASSERT( !path.empty() );

    TAC_CALL_RET( {}, const String str{ LoadAssetPath( path, errors ) } );

    Json json;
    TAC_CALL_RET( {}, json.Parse( str, errors ) );

    ShaderGraph sg { ShaderGraph::FromJson( json ) };
    return sg;
  }
} // namespace Tac

