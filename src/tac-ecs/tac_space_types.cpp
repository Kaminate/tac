#include "tac_space_types.h" // self-inc
#include "tac-std-lib/meta/tac_meta_integral.h"
#include "tac-std-lib/meta/tac_meta_impl.h"

namespace Tac
{
  static const MetaIntegralType< PlayerUUID > sPlayerUUIDMeta( "PlayerUUID" );
  static const MetaIntegralType< EntityUUID > sEntityUUIDMeta( "EntityUUID" );
  TAC_META_IMPL_INSTANCE( PlayerUUID, sPlayerUUIDMeta );
  TAC_META_IMPL_INSTANCE( EntityUUID, sEntityUUIDMeta );
}

