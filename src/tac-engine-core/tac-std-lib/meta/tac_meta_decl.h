#pragma once

namespace Tac
{
  struct MetaType;
#define TAC_META_DECL( T ) const MetaType& GetMetaType( const T& )
}

