#pragma once

//#include "tac-std-lib/meta/tac_meta.h"
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

#define TAC_REQUIRE_SEMICOLON           void missing_semicolon()

  // The macros
  // - TAC_META_REGISTER_COMPOSITE_BEGIN
  // - TAC_META_REGISTER_COMPOSITE_MEMBER
  // - TAC_META_REGISTER_COMPOSITE_END
  // are used to overload GetMetaType(const T&) to be used by GetMetaType<T>()

// Step 1 of 3: Declare a struct and open its ctor
#define TAC_META_REGISTER_COMPOSITE_BEGIN( T )                                                     \
  struct TAC_META_TYPE_NAME( T ) : public MetaCompositeType /* Open the struct */                  \
  {                                                                                                \
    TAC_META_TYPE_NAME( T )() : MetaCompositeType( #T, sizeof( T ), /* Open the ctor*/             \
      { /* Open the metamembers */

// Step 2 of 3: Define a vector of metamembers
#define TAC_META_REGISTER_COMPOSITE_MEMBER( T, M ) TAC_META_MEMBER( T, M ),

// Step 3 of 3
#define TAC_META_REGISTER_COMPOSITE_END( T )                                                       \
      } /* Close the metamembers */                                                                \
    ){ } /* Close the ctor */                                                                      \
  }; /* Close the struct */                                                                        \
  TAC_META_IMPL( T ); /* Define the GetMetaType() fn */                                            \
  TAC_REQUIRE_SEMICOLON


  void MetaCompositeUnitTest();

} // namespace Tac

