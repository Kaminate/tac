#include "tac_platform.h" // self-inc

namespace Tac
{
  static PlatformFns* sInstance;

  auto PlatformFns::GetInstance() -> PlatformFns* { return sInstance; }
  void PlatformFns::SetInstance( PlatformFns* fns ) { sInstance = fns; }

} // namespace Tac
