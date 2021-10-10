#include "src/common/meta/tacmetacomposite.h"
#include "src/common/tacJson.h"

#include <iostream>
#include <iomanip>

namespace Tac
{

  MetaMember::MetaMember( const char* name, int offset, const MetaType* metaType ) :
    mName( name ),
    mOffset( offset ),
    mMetaType( metaType ){}

  MetaCompositeType::MetaCompositeType( const char* name, int size, Vector< MetaMember > metaVars ) :
    mName( name ),
    mSize( size ),
    mMetaVars( metaVars )
  {
  }

  const char*       MetaCompositeType::GetName() const
  {
    return mName;
  }

  size_t            MetaCompositeType::GetSizeOf() const
  {
    return mSize;
  }

  const char*       MetaCompositeType::ToString( const void* ) const
  {
    TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
    return 0;
  }

  float             MetaCompositeType::ToNumber( const void* ) const
  {
    TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
    return 0;
  }

  void              MetaCompositeType::Cast( void* dst, const void* src, const MetaType* srcType ) const
  {
    TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
  }

  const MetaMember& MetaCompositeType::GetMember( int i ) const {
    return mMetaVars[ i ];
  }

  int               MetaCompositeType::GetMemberCount() const
  {
    return mMetaVars.size();
  }

  void              MetaCompositeType::JsonSerialize( Json* json, const void* v ) const
  {
    for( int iMember = 0; iMember < GetMemberCount(); ++iMember )
    {
      const MetaMember& member = GetMember( iMember );
      void* settingBytes = member.mOffset + ( char* )v;
      Json& child = json->GetChild( member.mName );
      member.mMetaType->JsonDeserialize( &child, settingBytes );
    }
  }

  void              MetaCompositeType::JsonDeserialize( const Json* json, void* v ) const
  {
    for( int iMember = 0; iMember < GetMemberCount(); ++iMember )
    {
      const MetaMember& member = GetMember( iMember );
      if( Json* child = json->FindChild( member.mName ) )
      {
        member.mMetaType->JsonDeserialize( child, member.mOffset + ( char* )v );
      }
    }
  }

  //===------------ Unit Test ------------===//

  struct MetaCompositeTestStruct
  {
    int foo;
    float bar;
  };
  TAC_META_REGISTER_COMPOSITE_BEGIN( MetaCompositeTestStruct )
  TAC_META_REGISTER_COMPOSITE_MEMBER( MetaCompositeTestStruct, foo )
  TAC_META_REGISTER_COMPOSITE_MEMBER( MetaCompositeTestStruct, bar )
  TAC_META_REGISTER_COMPOSITE_END( MetaCompositeTestStruct )
  void MetaCompositeUnitTest()
  {
    TAC_META_UNIT_TEST_TITLE;
    const MetaCompositeType& metaStruct = ( MetaCompositeType& )GetMetaType< MetaCompositeTestStruct >();
    for( int i = 0; i < metaStruct.GetMemberCount(); ++i )
    {
      auto member = metaStruct.GetMember( i );
      std::cout
        << std::setw( 5 )
        << member.mMetaType->GetName()
        << " "
        << metaStruct.GetName()
        << "::"
        << member.mName
        << std::endl;
    }
  }


} // namespace Tac

