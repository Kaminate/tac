// for common stuff used by
// - tac_renderer_directx11
// - tac_renderer_directx12

#pragma once

#include "src/common/preprocess/tac_preprocessor.h" // Tac::move

#include <unknwn.h> // IUnknown
#include <guiddef.h>

namespace Tac::Render
{
  // Wrapper for Com Object, similar to Microsoft::WRL::ComPtr
  template< typename T >
  struct PCom
  {
    // Constructors
    PCom() = default;
    PCom( T* ) = delete;
    PCom( const PCom& other )
    {
      mT = other.mT;
      TryAddRef();
    }

    PCom( PCom&& other )
    {
      swap( Tac::move(other) );
    }

    ~PCom()
    {
      TryRelease();
    }


    // IID_PPV_ARGS
    REFIID iid()          { return __uuidof(*mT); }
    void** ppv()          { return (void**)&mT; }

    // Member functions
    T**    CreateAddress() { return &mT; } // used during creation by a typed api
    T*     Get()           { return mT; }

    //void   clear()         { TryRelease(); }
    void swap( PCom&& other )
    {
      T* t = mT;
      mT = other.mT;
      other.mT = t;
    }

    // Query Interface

    template< typename U >
    PCom< U > QueryInterface()
    {
      PCom< U > u;
      QueryInterface( u );
      return u;
    }

    template< typename U >
    void QueryInterface( PCom<U>& u )
    {
      if( IUnknown* unknown = GetUnknown() )
        unknown->QueryInterface( u.iid(), u.ppv() );
    }

    // arrow oprator

    T*       operator ->()       { return mT; }
    const T* operator ->() const { return mT; }

    // Conversion operators

    template< typename U > explicit operator U* ( ) const { return static_cast< U* >( mT ); }

    operator bool() const { return mT; }

    // Assignment operators

    void operator = (T*) = delete;

    void operator = ( const PCom& other )
    {
      mT = other.mT;
      TryAddRef();
    }

    void operator = ( PCom&& other )
    {
      swap( Tac::move( other ) );
      //TryRelease();
      //mT = other.mT;
      //other.mT = nullptr;
    }


  private:

    IUnknown* GetUnknown()
    {
      return static_cast<IUnknown*>(mT);
    }

    void TryAddRef()
    {
      if( IUnknown* unknown = GetUnknown() )
        unknown->AddRef();
    }

    void TryRelease()
    {
      if( IUnknown* unknown = GetUnknown() )
        unknown->Release();
      mT = nullptr;
    }
    
    T* mT = nullptr;
  };

} // namespace Tac::Render

