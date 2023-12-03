#include "src/shell/windows/renderer/dxgi/tac_dxgi_debug.h" // self-inc

#include "src/shell/windows/renderer/tac_dx.h"

#include <dxgidebug.h>

namespace Tac::Render
{

  static PCom<IDXGIDebug> GetDXGIDebug()
  {
    if( !IsDebugMode )
      return {};

    HMODULE hModule = GetModuleHandle( "Dxgidebug.dll" );
    if( !hModule )
      return {};

    using GetDXGIFn = HRESULT( WINAPI* )( REFIID, void** );

    FARPROC fnAddr = GetProcAddress( hModule, "DXGIGetDebugInterface" );
    if( !fnAddr )
      return {};

    GetDXGIFn fn = (GetDXGIFn)fnAddr;

    PCom<IDXGIDebug> dxgiDbg;
    const HRESULT hr = fn( dxgiDbg.iid(), dxgiDbg.ppv() );
    if( FAILED( hr ) )
      return {};

    return dxgiDbg;
  }

  void DXGIReportLiveObjects()
  {
    if( !IsDebugMode )
      return;

    PCom<IDXGIDebug> dxgiDbg = GetDXGIDebug();
    if( !dxgiDbg )
      return;

#if 0
    TAC_SCOPE_GUARD( ScopedSeverityBreak );
#endif

    dxgiDbg->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL );
  }
} // namespace Tac::Render
