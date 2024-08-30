#pragma once

namespace Tac
{
  struct MetaType;

#if defined( TAC_GET_META_TYPE_DEFINED )
#error tac_meta.h must be included after
#else
#define TAC_META_DECL( T ) const MetaType& GetMetaType( const T& )
#endif

} // namespace Tac

