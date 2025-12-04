#pragma once

#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_decl.h"
#include "tac-std-lib/meta/tac_meta_impl.h"
#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{
  struct MetaMember
  {
    StringView            mName     {};
    int                   mOffset   {};
    const MetaType*       mMetaType {};
  };

#define TAC_META_MEMBER( T, M )                                                                    \
  MetaMember                                                                                       \
  {                                                                                                \
    .mName     { #M },                                                                             \
    .mOffset   { TAC_OFFSET_OF( T, M ) },                                                          \
    .mMetaType { &GetMetaType< decltype( T::M ) >() },                                             \
  }

  struct MetaCompositeType : public MetaType
  {
    MetaCompositeType() = default;
    //MetaCompositeType( const char* name, int size, Vector< MetaMember > metaVars );
    const char*           GetName() const override;
    void                  SetName( const char* );
    void                  SetSize( int );
    void                  AddMetaMember( MetaMember );
    int                   GetSizeOf() const override;
    String                ToString( const void* ) const override;
    float                 ToNumber( const void* ) const override;
    void                  Cast( CastParams ) const override;
    const MetaMember&     GetMember( int ) const;
    int                   GetMemberCount() const;
    void                  JsonSerialize( Json* json, const void* v ) const override;
    void                  JsonDeserialize( const Json* json, void* v ) const override;
    bool                  Equals( const void*, const void* ) const override;
    void                  Copy( CopyParams ) const override;

  protected:
    Vector< MetaMember >  mMetaVars {};
    const char*           mName     {};
    int                   mSize     {};
  };

#define TAC_REQUIRE_SEMICOLON           void missing_semicolon()

  // The macros
  // - TAC_META_REGISTER_COMPOSITE_BEGIN
  // - TAC_META_REGISTER_COMPOSITE_MEMBER
  // - TAC_META_REGISTER_COMPOSITE_END
  // are used to overload GetMetaType(const T&) to be used by GetMetaType<T>()

// Step 1 of 3: Declare a struct and open its ctor
#define TAC_META_REGISTER_COMPOSITE_BEGIN( T )                                                     \
  struct TAC_META_TYPE_NAME( T ) : public MetaCompositeType                                        \
  {                                                                                                \
    using BaseType = T;                                                                            \
    TAC_META_TYPE_NAME( T )()                                                                      \
    {                                                                                              \
      MetaCompositeType::SetName( #T );                                                            \
      MetaCompositeType::SetSize( sizeof( T ) );                                                   \
      TAC_REQUIRE_SEMICOLON


// Step 2 of 3: Define a vector of metamembers
#define TAC_META_REGISTER_COMPOSITE_MEMBER( M )                                                    \
      MetaCompositeType::AddMetaMember( TAC_META_MEMBER( BaseType, M ) );                          \
      TAC_REQUIRE_SEMICOLON

// Step 3 of 3
#define TAC_META_REGISTER_COMPOSITE_END( T )                                                       \
    }                                                                                              \
  };                                                                                               \
  TAC_META_IMPL( T ); /* Define the GetMetaType() fn */                                            \
  TAC_REQUIRE_SEMICOLON


#define TAC_META_REGISTER_STRUCT_BEGIN( T )  TAC_META_REGISTER_COMPOSITE_BEGIN( T )
#define TAC_META_REGISTER_STRUCT_MEMBER( M ) TAC_META_REGISTER_COMPOSITE_MEMBER( M )
#define TAC_META_REGISTER_STRUCT_END( T )    TAC_META_REGISTER_COMPOSITE_END( T )

#define TAC_META_REGISTER_CLASS_BEGIN( T )   TAC_META_REGISTER_COMPOSITE_BEGIN( T )
#define TAC_META_REGISTER_CLASS_MEMBER( M )  TAC_META_REGISTER_COMPOSITE_MEMBER( M )
#define TAC_META_REGISTER_CLASS_END( T )     TAC_META_REGISTER_COMPOSITE_END( T )

  void MetaCompositeUnitTest();

} // namespace Tac

