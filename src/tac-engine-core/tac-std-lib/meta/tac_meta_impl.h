#pragma once

#include "tac-std-lib/meta/tac_meta_decl.h"
#include "tac-std-lib/meta/tac_meta_type.h"

namespace Tac
{
  struct MetaType;


#define TAC_META_INSTANCE_NAME( T ) s##Meta##T
#define TAC_META_TYPE_NAME( T )     Meta##T

#define TAC_META_IMPL_INSTANCE( T, sT ) TAC_META_DECL( T ) { return sT; }
#define TAC_META_IMPL_INSTANCE2( T, MetaT ) TAC_META_DECL( T ) { static const MetaT sMetaType; return sMetaType; }

#define TAC_META_IMPL_TYPE( T, MetaT )                                     \
  static MetaT TAC_META_INSTANCE_NAME( T );                                \
  TAC_META_IMPL_INSTANCE( T, TAC_META_INSTANCE_NAME( T ) )

#define TAC_META_IMPL( T )                                                 \
  static const TAC_META_TYPE_NAME( T ) TAC_META_INSTANCE_NAME( T );        \
  TAC_META_IMPL_INSTANCE( T, TAC_META_INSTANCE_NAME( T ) )


} // namespace Tac

