#include "tac_material_input.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{

  static StringView          ToString( MaterialInput::Type mi )
  {
    switch( mi )
    {
    case MaterialInput::Type::kWorldMatrix: return "world_matrix";
    case MaterialInput::Type::kVertexBuffer: return "vertex_buffer";
    default: TAC_ASSERT_INVALID_CASE( mi ); return "";
    }
  }

  static MaterialInput::Type FromString( StringView s )
  {
    for( int i{}; i < ( int )MaterialInput::Type::kCount; ++i )
    {
      const MaterialInput::Type type{ ( MaterialInput::Type )i };
      if( ( StringView )ToString( type ) == s )
        return type;
    }
    TAC_ASSERT_INVALID_CODE_PATH;
    return MaterialInput::Type::kCount;
  }

  // -----------------------------------------------------------------------------------------------

  MaterialInput       MaterialInput::FromJson( const Json* materialInputsJson )
  {
    MaterialInput materialInputs{};
    if( materialInputsJson )
    {
      for( Json* varJson : materialInputsJson->mArrayElements )
      {
        materialInputs.Set( FromString( varJson->mString ) );
      }
    }

    return materialInputs;
  }

  Json                MaterialInput::ToJson( const MaterialInput& materialInputs )
  {
    Json json;
    const int n{ ( int )MaterialInput::Type::kCount };
    for( int i{}; i < n; ++i )
    {
      const MaterialInput::Type type{ ( MaterialInput::Type )i };
      if( materialInputs.IsSet( type ) )
      {
        *json.AddChild() = ToString( type );
      }
    }

    return json;
  }

  // -----------------------------------------------------------------------------------------------

  void MaterialInput::Set( Type t )
  {
    ( UnderlyingType& )mBitfield |= ( UnderlyingType )( 1 << t );
  }

  void MaterialInput::Set( Type t, bool b )
  {
    if( b ) Set( t ); else Clear( t );
  }

  void MaterialInput::Clear( Type t )
  {
    ( UnderlyingType& )mBitfield &= ~( 1 << t );
  }

  void MaterialInput::Clear()
  {
    mBitfield = {};
  }

  bool MaterialInput::IsSet( Type t ) const
  {
    return ( UnderlyingType )mBitfield & ( 1 << t );
  }

} // namespace Tac

