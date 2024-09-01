#include "tac_meta_type.h"

namespace Tac
{
  void        MetaType::Read( ReadStream*, void* ) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void        MetaType::Write( WriteStream*, const void* ) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  bool        MetaType::Equals( const void*, const void* ) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return false;
  }

  void        MetaType::Copy( CopyParams ) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

} // namespace Tac

