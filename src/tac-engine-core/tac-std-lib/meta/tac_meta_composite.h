#pragma once

#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{
  struct MetaMember
  {
    MetaMember() = default;
    MetaMember( const char* name, int offset, const MetaType* metaType );
    StringView            mName;
    int                   mOffset {};
    const MetaType*       mMetaType {};
  };

#define TAC_META_MEMBER( T, M ) MetaMember( #M, TAC_OFFSET_OF( T, M ), &GetMetaType< decltype( T::M ) >() )

  struct MetaCompositeType : public MetaType
  {
    MetaCompositeType( const char* name, int size, Vector< MetaMember > metaVars );
    const char*           GetName() const override;
    int                   GetSizeOf() const override;
    String                ToString( const void* ) const override;
    float                 ToNumber( const void* ) const override;
    void                  Cast( CastParams ) const override;
    const MetaMember&     GetMember( int ) const;
    int                   GetMemberCount() const;
    void                  JsonSerialize( Json* json, const void* v ) const override;
    void                  JsonDeserialize( const Json* json, void* v ) const override;
  private:
    Vector< MetaMember >  mMetaVars;
    const char*           mName;
    int                   mSize;
  };

#define TAC_REQUIRE_SEMICOLON void missing_semicolon()

#define TAC_META_DECLARE_COMPOSITE( T ) const MetaCompositeType& GetMetaType( const T& )

#define TAC_META_COMPOSITE_NAME( T ) s##T##MetaType


  // The macros
  // - TAC_META_REGISTER_COMPOSITE_BEGIN
  // - TAC_META_REGISTER_COMPOSITE_MEMBER
  // - TAC_META_REGISTER_COMPOSITE_END
  // are used to overload GetMetaType(const T&) to be used by GetMetaType<T>()
#define TAC_META_REGISTER_COMPOSITE_BEGIN_alt( T, members ) 
#define TAC_META_REGISTER_COMPOSITE_BEGIN( T )                                                 \
  static MetaCompositeType TAC_META_COMPOSITE_NAME( T )(                                       \
    #T,                                                                                        \
    sizeof( T ),                                                                               \
    {

#define TAC_META_REGISTER_COMPOSITE_MEMBER( T, M )                                             \
        TAC_META_MEMBER( T, M ),

#define TAC_META_REGISTER_COMPOSITE_END( T )                                                   \
    } );                                                                                       \
  const MetaCompositeType& GetMetaType( const T& ) { return TAC_META_COMPOSITE_NAME( T ); }    \
  TAC_REQUIRE_SEMICOLON


  void MetaCompositeUnitTest();

} // namespace Tac

