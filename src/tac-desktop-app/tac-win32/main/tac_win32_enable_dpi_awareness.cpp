#include "tac_win32_main.h"
#include "tac_win32.h"

#ifndef DPI_ENUMS_DECLARED
typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

void Tac::Win32EnableDpiAwareness()
{
  if( _IsWindows10OrGreater() )
    if( static HINSTANCE user32_dll{ ::LoadLibraryA( "user32.dll" ) }; user32_dll )
      if( auto fn{ ( PFN_SetThreadDpiAwarenessContext )::GetProcAddress( user32_dll, "SetThreadDpiAwarenessContext" ) } )
        if( fn( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ); true )
          return;

  if( _IsWindows8Point1OrGreater() )
    if( static HINSTANCE shcore_dll{ ::LoadLibraryA( "shcore.dll" ) }; shcore_dll )
      if( auto fn{ ( PFN_SetProcessDpiAwareness )::GetProcAddress( shcore_dll, "SetProcessDpiAwareness" ) } )
        if( fn( PROCESS_PER_MONITOR_DPI_AWARE ); true )
          return;

#if _WIN32_WINNT >= 0x0600
  ::SetProcessDPIAware();
#endif

}
