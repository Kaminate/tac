#include "tac_meta_composite.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/meta/tac_meta.h"
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

  int               MetaCompositeType::GetSizeOf() const
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

  void              MetaCompositeType::Cast( CastParams ) const
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
    for( int iMember {}; iMember < GetMemberCount(); ++iMember )
    {
      const MetaMember& member { GetMember( iMember ) };
      void* settingBytes { member.mOffset + ( char* )v };
      Json& child { json->GetChild( member.mName ) };
      member.mMetaType->JsonDeserialize( &child, settingBytes );
    }
  }

  void              MetaCompositeType::JsonDeserialize( const Json* json, void* v ) const
  {
    for( int iMember {}; iMember < GetMemberCount(); ++iMember )
    {
      const MetaMember& member { GetMember( iMember ) };
      if( Json * child{ json->FindChild( member.mName ) } )
      {
        member.mMetaType->JsonDeserialize( child, member.mOffset + ( char* )v );
      }
    }
  }

  bool              MetaCompositeType::Equals( const void* a, const void* b ) const
  {
    for( const MetaMember& metaMember : mMetaVars )
    {
      const void* aMember{ ( const char* )a + metaMember.mOffset };
      const void* bMember{ ( const char* )b + metaMember.mOffset };
      if( !metaMember.mMetaType->Equals( aMember, bMember ) )
        return false;
    }

    return true;
  }

  void              MetaCompositeType::Copy( CopyParams copyParams ) const
  {
    for( const MetaMember& metaMember : mMetaVars )
    {
      dynmc void* dstMember{ ( dynmc char* )copyParams.mDst + metaMember.mOffset };
      const void* srcMember{ ( const char* )copyParams.mSrc + metaMember.mOffset };
      const CopyParams memberCopyParams
      {
        .mDst{dstMember},
        .mSrc{srcMember},
      };
      metaMember.mMetaType->Copy( memberCopyParams );
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
    for( int i{}; i < metaStruct.GetMemberCount(); ++i )
    {
      const MetaMember& member { metaStruct.GetMember( i ) };
      const char* strType { member.mMetaType->GetName() };
      const char* strStruct { metaStruct.GetName() };
      const char* strMember { member.mName };
      OS::OSDebugPrintLine( String() + strType + " " + strStruct + ":: " + strMember );
    }
  }


} // namespace Tac

