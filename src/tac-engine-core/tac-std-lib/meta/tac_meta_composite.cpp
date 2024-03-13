#include "tac-std-lib/meta/tac_meta_composite.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/os/tac_os.h"

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

  String            MetaCompositeType::ToString( const void* ) const
  {
    TAC_ASSERT_INVALID_CODE_PATH;
    return 0;
  }

  float             MetaCompositeType::ToNumber( const void* ) const
  {
    TAC_ASSERT_INVALID_CODE_PATH;
    return 0;
  }

  void              MetaCompositeType::Cast( void* dst, const void* src, const MetaType* srcType ) const
  {
    TAC_ASSERT_INVALID_CODE_PATH;
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
    int   foo;
    float bar;
  };

  TAC_META_REGISTER_COMPOSITE_BEGIN( MetaCompositeTestStruct )
    TAC_META_REGISTER_COMPOSITE_MEMBER( MetaCompositeTestStruct, foo )
    TAC_META_REGISTER_COMPOSITE_MEMBER( MetaCompositeTestStruct, bar )
    TAC_META_REGISTER_COMPOSITE_END( MetaCompositeTestStruct );

  void MetaCompositeUnitTest()
  {
    const MetaCompositeType& metaStruct = ( MetaCompositeType& )GetMetaType< MetaCompositeTestStruct >();
    for( int i = 0; i < metaStruct.GetMemberCount(); ++i )
    {
      const MetaMember& member = metaStruct.GetMember( i );
      const char* strType = member.mMetaType->GetName();
      const char* strStruct = metaStruct.GetName();
      const char* strMember = member.mName;
      OS::OSDebugPrintLine( String() + strType + " " + strStruct + ":: " + strMember );
    }
  }


} // namespace Tac

