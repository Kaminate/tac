#include "tac_desktop_app_threads.h"

#include "src/common/error/tac_error_handling.h"

namespace Tac
{

  thread_local DesktopAppThreads::ThreadType gThreadType = DesktopAppThreads::ThreadType::Unknown;

  void DesktopAppThreads::SetType( ThreadType type )
  {
    TAC_ASSERT( gThreadType == DesktopAppThreads::ThreadType::Unknown );
    gThreadType = type;
  }

  bool DesktopAppThreads::IsType( ThreadType type )
  {
    return gThreadType == type;
  }

  bool DesktopAppThreads::IsMainThread() { return IsType( ThreadType::Main ); }
  bool DesktopAppThreads::IsLogicThread() { return IsType( ThreadType::Logic ); }
} // namespace Tac
