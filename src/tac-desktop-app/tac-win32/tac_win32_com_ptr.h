#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h" // Tac::move

#include <unknwn.h> // IUnknown
#include <guiddef.h>

// delete be begin
#include <d3d12.h>
// delete be end

namespace Tac
{
  // Wrapper for Com Object, similar to Microsoft::WRL::ComPtr
  template< typename T >
  struct PCom
  {
    // Constructors
    PCom() = default;

    // why delete this?
    PCom( T* ) = delete;

    PCom( const PCom& other )
    {
      mT = other.mT;
      TryAddRef();
    }

    PCom( PCom&& other ) noexcept
    {
      swap( Tac::move( other ) );
    }

    ~PCom()
    {
      TryRelease();
    }

    // IID_PPV_ARGS == iid(), ppv()
    REFIID iid()          { return __uuidof(*mT); }
    void** ppv()          { return (void**)&mT; }

    // Member functions
    T**    CreateAddress() { return &mT; } // used during creation by a typed api
    T*     Get()           { return mT; }
    T*     Get() const     { return mT; }

    void swap( PCom&& other )
    {
      T* t{ mT };
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
      // If the call to QueryInterface is successful, IUnknown::AddRef is automatically
      // called on the COM object. Otherwise, the pointed address is set to nullptr
      if( IUnknown * unknown { static_cast< IUnknown* >( mT ) } )
        unknown->QueryInterface( u.iid(), u.ppv() );
    }

    // arrow oprator

    T*       operator ->()       { return mT; }
    const T* operator ->() const { return mT; }

    // Conversion operators

    template< typename U > explicit operator U* ( ) const { return static_cast< U* >( mT ); }

    operator bool() const { return mT; }

    // Assignment operators

    //void operator = (T*) = delete;
    // im undeleting this idk why it was deleted
    void operator = ( T* t )
    {
      TryRelease();
      mT = t;
      TryAddRef();
    }

    void operator = ( const PCom& other )
    {
      TryRelease();
      mT = other.mT;
      TryAddRef();
    }

    void operator = ( PCom&& other ) noexcept
    {
      swap( Tac::move( other ) );
    }


  private:

    void TryAddRef()
    {
      if( IUnknown * unknown{ static_cast< IUnknown* >( mT ) } )
        unknown->AddRef();
    }

    void TryRelease()
    {
      if( IUnknown * unknown{ static_cast< IUnknown* >( mT ) } )
        unknown->Release();
    }
    
    T* mT {};
  };

} // namespace Tac

