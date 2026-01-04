#include "tac_meta_composite.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  void MetaCompositeType::SetName( const char* name )    { mName = name; }
  void MetaCompositeType::SetSize( int size )            { mSize = size; }
  void MetaCompositeType::AddMetaMember( MetaMember m )  { mMetaVars.push_back( m ); }
  auto MetaCompositeType::GetName() const -> const char* { return mName; }
  auto MetaCompositeType::GetSizeOf() const -> int       { return mSize; }

  auto MetaCompositeType::ToString( const void* ) const -> String
  {
    TAC_ASSERT_INVALID_CODE_PATH;
    return 0;
  }

  auto MetaCompositeType::ToNumber( const void* ) const -> float
  {
    TAC_ASSERT_INVALID_CODE_PATH;
    return 0;
  }

  void MetaCompositeType::Cast( const CastParams castParams ) const
  {
    if( castParams.mSrcType == this )
    {
      const CopyParams copyParams
      {
        .mDst { castParams.mDst },
        .mSrc { castParams.mSrc },
      };
      Copy( copyParams );
      return;
    }
    
    TAC_ASSERT_INVALID_CODE_PATH;
  }

  auto MetaCompositeType::GetMember( int i ) const -> const MetaMember& { return mMetaVars[ i ]; }

  auto MetaCompositeType::GetMemberCount() const -> int { return mMetaVars.size(); }

  void MetaCompositeType::JsonSerialize( Json* json, const void* v ) const
  {
    const int n{ GetMemberCount() };
    for( int i {}; i < n; ++i )
    {
      const MetaMember& member { GetMember( i ) };
      void* settingBytes { member.mOffset + ( char* )v };
      Json& child { json->GetChild( member.mName ) };
      member.mMetaType->JsonSerialize( &child, settingBytes );
    }
  }

  void MetaCompositeType::JsonDeserialize( const Json* json, void* v ) const
  {
    const int n{ GetMemberCount() };
    for( int i {}; i < n; ++i )
    {
      const MetaMember& member { GetMember( i ) };
      if( Json * child{ json->FindChild( member.mName ) } )
      {
        member.mMetaType->JsonDeserialize( child, member.mOffset + ( char* )v );
      }
    }
  }

  bool MetaCompositeType::Equals( const void* a, const void* b ) const
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

  void MetaCompositeType::Copy( CopyParams copyParams ) const
  {
    for( const MetaMember& metaMember : mMetaVars )
    {
      const CopyParams memberCopyParams
      {
        .mDst { ( dynmc char* )copyParams.mDst + metaMember.mOffset  },
        .mSrc { ( const char* )copyParams.mSrc + metaMember.mOffset  },
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

  TAC_META_REGISTER_STRUCT_BEGIN( MetaCompositeTestStruct );
  TAC_META_REGISTER_STRUCT_MEMBER( foo );
  TAC_META_REGISTER_STRUCT_MEMBER( bar );
  TAC_META_REGISTER_STRUCT_END( MetaCompositeTestStruct );

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

