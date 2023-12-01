// for common stuff used by
// - tac_renderer_directx11
// - tac_renderer_directx12

#pragma once

//#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"

#include <unknwn.h> // IUnknown
#include <guiddef.h>

import std;

namespace Tac::Render
{
  // Wrapper for Com Object, similar to Microsoft::WRL::ComPtr
  template< typename T > requires std::is_base_of_v< IUnknown, T >
  struct PCom
  {
    ~PCom()
    {
      if( IUnknown* iUk = mT )
      {
        iUk->Release();
        mT = nullptr;
      }
    }

    REFIID id() { return __uuidof(*mT); }
    void** pp() { return (void**)&mT; }

    template< typename U > // requires std::is_base_of_v< IUnknown, U >
    PCom< U > QueryInterface()
    {
      if( IUnknown* iUk = mT )
      {
        PCom< U > pComU;
        iUk->QueryInterface( pComU.id(), pComU.pp() ); // ignore hr
        return pComU;
      }
    }

    void operator = ( T* t )
    {
      TAC_ASSERT( !mT );
      mT = t;
    }
    
    operator T* ( )
    {
      return mT;
    }

    T* operator ->()
    {
      return mT;
    }

  private:
    
    T* mT = nullptr;
  };

} // namespace Tac::Render

