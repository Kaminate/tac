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
    //PCom( T* t = nullptr ) : mT( t ) {}

    PCom() = default;
    PCom( const PCom& other )
    {
      mT = other.mT;
      TryAddRef();
    }

    PCom( PCom&& other )
    {
      mT = other.mT;
      other.mT = nullptr;
    }

    ~PCom()
    {
      TryRelease();
    }
    
    //void clear()
    //{
    //  TryRelease();
    //  if( IUnknown* iUk = mT )
    //  {
    //    iUk->Release();
    //    mT = nullptr;
    //  }
    //}

    operator bool() const { return mT; }

    REFIID iid()     { return __uuidof(*mT); }
    void** ppv()     { return (void**)&mT; }
    T**    pp()      { return &mT; }
    explicit operator T* ()   { return mT; }
    T* operator ->() { return mT; }


    template< typename U > // requires std::is_base_of_v< IUnknown, U >
    PCom< U > QueryInterface()
    {
      PCom< U > u;
      if( IUnknown* iUk = mT)
        iUk->QueryInterface( u.iid(), u.ppv() );
      return u;
    }


    void operator = (T*) = delete;

    void operator = ( const PCom& other )
    {
      mT = other.mT;
      TryAddRef();
    }

    void operator = ( PCom&& other )
    {
      TryRelease();
      mT = other.mT;
      other.mT = nullptr;
    }

    //void operator = ( T* t )
    //{
    //  TAC_ASSERT( !mT );
    //  mT = t;
    //}
    
    PCom( T* ) = delete;

  private:

    void TryAddRef()
    {
      if( IUnknown* iUk = mT )
        iUk->AddRef();
    }

    void TryRelease()
    {
      if( IUnknown* iUk = mT )
        iUk->Release();
      mT = nullptr;
    }
    
    //PCom( T* t ) : mT( t ) {};
    T* mT = nullptr;
  };

} // namespace Tac::Render

