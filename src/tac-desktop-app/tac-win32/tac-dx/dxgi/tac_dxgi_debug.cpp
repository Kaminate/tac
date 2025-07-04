#include "tac_dxgi_debug.h" // self-inc

#include "tac-win32/tac_win32_com_ptr.h"

#include <dxgidebug.h>
#include <dxgi1_3.h> // DXGIGetDebugInterface1

#pragma comment( lib, "DXGI.lib" )

namespace Tac::Render
{
  struct DXGIDebugWrapper
  {
    void Init();
    void Report();

    PCom<IDXGIDebug> mDbg;
  };

  void DXGIDebugWrapper::Init()
  {
    if constexpr( kIsDebugMode )
    {

      const UINT flags{};
      DXGIGetDebugInterface1( flags, mDbg.iid(), mDbg.ppv() );
    }
  }

  void DXGIDebugWrapper::Report()
  {
    if constexpr( kIsDebugMode )
    {

      if( !mDbg )
        return;

#if 0
      TAC_SCOPE_GUARD( ScopedSeverityBreak );
#endif

      mDbg->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL );
    }
  }


  void DXGIReportLiveObjects()
  {
    if constexpr( kIsDebugMode )
    {

      DXGIDebugWrapper wrapper;
      wrapper.Init();
      wrapper.Report();
    }

  }
} // namespace Tac::Render
