#include "tac_shader_graph.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  static const char* sJsonKeyMaterialVSOut   { "material_vs_out" };
  static const char* sJsonKeyMaterialInputs  { "material_inputs" };
  static const char* sJsonKeyMaterialShader  { "material_shader" };

  // -----------------------------------------------------------------------------------------------

  Json        ShaderGraph::JsonSave( const ShaderGraph& sg )
  {
    Json json;
    json[ sJsonKeyMaterialVSOut ] = MaterialVSOut::ToJson( sg.mMaterialVSOut );
    json[ sJsonKeyMaterialInputs ] = MaterialInput::JsonSave( sg.mMaterialInputs );
    json[ sJsonKeyMaterialShader ] = ( StringView )sg.mMaterialShader;
    return json;
  }

  ShaderGraph ShaderGraph::JsonLoad( const Json& json )
  {
    const Json* materialShaderJson{ json.FindChild( sJsonKeyMaterialShader ) };
    const Json* inputLayoutJson   { json.FindChild( sJsonKeyMaterialVSOut ) };
    const Json* materialInputsJson{ json.FindChild( sJsonKeyMaterialInputs ) };
    const MaterialVSOut materialVSOut{ MaterialVSOut::FromJson( inputLayoutJson ) };
    const MaterialInput materialInputs{ MaterialInput::JsonLoad( materialInputsJson ) };
    const StringView materialShader{ materialShaderJson
      ? StringView( materialShaderJson->mString )
      : StringView{} };
    return ShaderGraph
    {
      .mMaterialVSOut       { materialVSOut },
      .mMaterialInputs      { materialInputs },
      .mMaterialShader      { materialShader },
    };
  }

  void        ShaderGraph::FileSave( const ShaderGraph& sg,
                                   AssetPathStringView path,
                                   Errors& errors )
  {
    TAC_ASSERT( !path.empty() );
    const Json json{ ShaderGraph::JsonSave( sg ) };
    const String string{ json.Stringify() };
    TAC_CALL( SaveToFile( path, string.data(), string.size(), errors ) );
  }

  ShaderGraph ShaderGraph::FileLoad( AssetPathStringView path, Errors& errors )
  {
    TAC_ASSERT( !path.empty() );
    TAC_ASSERT( path.starts_with( "assets/shader-graphs/" ) );
    TAC_CALL_RET( const String str{ LoadAssetPath( path, errors ) } );
    TAC_CALL_RET( const Json json{ Json::Parse( str, errors ) } );
    return ShaderGraph::JsonLoad( json );
  }
} // namespace Tac

