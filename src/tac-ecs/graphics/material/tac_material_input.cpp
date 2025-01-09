#include "tac_material_input.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{
  static StringView kStrWorldMatrix  { "world_matrix" };
  static StringView kStrVertexBuffer { "vertex_buffer" };
  static StringView kStrInputLayout  { "input_layout" };

  StringView          MaterialInput::Type_to_String( MaterialInput::Type mi )
  {
    switch( mi )
    {
    case MaterialInput::Type::kWorldMatrix:  return kStrWorldMatrix;
    case MaterialInput::Type::kVertexBuffer: return kStrVertexBuffer;
    case MaterialInput::Type::kInputLayout:  return kStrInputLayout;
    default: TAC_ASSERT_INVALID_CASE( mi );  return "";
    }
  }

  MaterialInput::Type MaterialInput::String_to_Type( StringView s )
  {
    if( s == kStrWorldMatrix )  return MaterialInput::Type::kWorldMatrix;
    if( s == kStrVertexBuffer ) return MaterialInput::Type::kVertexBuffer;
    if( s == kStrInputLayout )  return MaterialInput::Type::kInputLayout;
    TAC_ASSERT_INVALID_CODE_PATH;
    return MaterialInput::Type::kUnknown;
  }

  // -----------------------------------------------------------------------------------------------
  MaterialInput::TypeIterator MaterialInput::TypeIterator::begin() const { return { kFirstType }; }
  MaterialInput::TypeIterator MaterialInput::TypeIterator::end()   const
  {
    return { ( Type )( ( UnderlyingType )kLastType << 1 ) };
  }
  MaterialInput::Type         MaterialInput::TypeIterator::operator *() const { return mType; }
  void                        MaterialInput::TypeIterator::operator ++()       
  {
    mType = ( Type )( ( UnderlyingType )mType << 1 );
  }

  // -----------------------------------------------------------------------------------------------

  MaterialInput       MaterialInput::JsonLoad( const Json* materialInputsJson )
  {
    MaterialInput materialInputs{};
    if( materialInputsJson )
      for( Json* varJson : materialInputsJson->mArrayElements )
        materialInputs.Set( MaterialInput::String_to_Type( varJson->mString ) );

    return materialInputs;
  }

  Json                MaterialInput::JsonSave( const MaterialInput& materialInputs )
  {
    Json json;
    for( const MaterialInput::Type type : MaterialInput::TypeIterator() )
      if( materialInputs.IsSet( type ) )
        *json.AddChild() = MaterialInput::Type_to_String( type );

    //const int n{ ( int )MaterialInput::Type::kUnknown };
    //for( int i{}; i < n; ++i )
    //  if( const MaterialInput::Type type{ ( MaterialInput::Type )i };
    //      materialInputs.IsSet( type ) )
    //    *json.AddChild() = MaterialInput::Type_to_String( type );

    return json;
  }

  // -----------------------------------------------------------------------------------------------

  void MaterialInput::Set( Type t )
  {
    ( UnderlyingType& )mBitfield |= 1 << ( UnderlyingType )t;
  }

  void MaterialInput::Set( Type t, bool b )
  {
    if( b )
      Set( t );
    else
      Clear( t );
  }

  void MaterialInput::Clear( Type t )
  {
    ( UnderlyingType& )mBitfield &= ~( 1 << ( UnderlyingType )t );
  }

  void MaterialInput::Clear()
  {
    mBitfield = {};
  }

  bool MaterialInput::IsSet( Type t ) const
  {
    return ( UnderlyingType )mBitfield & ( 1 << ( UnderlyingType )t );
  }

} // namespace Tac

