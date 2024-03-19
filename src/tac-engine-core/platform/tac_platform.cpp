#include "tac_platform.h" // self-inc

namespace Tac
{
  static PlatformFns* sInstance;

  PlatformFns* PlatformFns::GetInstance()
  {
    return sInstance;
  }

  void         PlatformFns::SetInstance( PlatformFns* fns )
  {
    sInstance = fns;
  }


} // namespace Tac
