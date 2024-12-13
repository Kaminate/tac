#include "tac_desktop_app_threads.h"

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  thread_local DesktopAppThreads::ThreadType gThreadType{ DesktopAppThreads::ThreadType::Unknown };

  void DesktopAppThreads::SetType( ThreadType type )
  {
    TAC_ASSERT( gThreadType == DesktopAppThreads::ThreadType::Unknown );
    gThreadType = type;
  }

  bool DesktopAppThreads::IsType( ThreadType type ) { return gThreadType == type; }
  bool DesktopAppThreads::IsSysThread() { return IsType( ThreadType::Sys ); }
  bool DesktopAppThreads::IsSimThread() { return IsType( ThreadType::Sim ); }
  bool DesktopAppThreads::IsAppThread() { return IsType( ThreadType::Sim ); }
} // namespace Tac
