#pragma once

#include "src/common/meta/tacmeta.h"

namespace Tac
{
  struct MetaMember
  {
    MetaMember() = default;
    MetaMember( const char* name, int offset, const MetaType* metaType );
    StringView            mName;
    int                   mOffset = 0;
    const MetaType*       mMetaType = nullptr;
  };

  struct MetaCompositeType : public MetaType
  {
    MetaCompositeType( const char* name, int size, Vector< MetaMember > metaVars );
    const char*           GetName() const override;
    size_t                GetSizeOf() const override;
    const char*           ToString( void* ) const override;
    float                 ToNumber( void* ) const override;
    void                  Cast( void* dst, void* src, const MetaType* srcType ) const override;
    const MetaMember&     GetMember( int ) const;
    int                   GetMemberCount() const;
  private:
    Vector< MetaMember >  mMetaVars;
    const char*           mName;
    size_t                mSize;
  };

#define TAC_META_DECLARE_COMPOSITE( T ) const MetaCompositeType& GetMetaType( const T& );

#define TAC_META_COMPOSITE_NAME( T ) s##T##MetaType

#define TAC_META_REGISTER_COMPOSITE_BEGIN( T )                                                 \
  static MetaCompositeType TAC_META_COMPOSITE_NAME( T )(                                       \
    #T,                                                                                        \
    sizeof( T ),                                                                               \
    {

#define TAC_META_REGISTER_COMPOSITE_MEMBER( T, M )                                             \
      MetaMember( #M,                                                                          \
                  TAC_OFFSET_OF( T, M ),                                                       \
                  &GetMetaType< decltype( T::M ) >() ),

#define TAC_META_REGISTER_COMPOSITE_END( T )                                                   \
    } );                                                                                       \
  const MetaCompositeType& GetMetaType( const T& ) { return TAC_META_COMPOSITE_NAME( T ); }


  void MetaCompositeUnitTest();

} // namespace Tac

