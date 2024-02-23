#pragma once

#include "src/common/preprocess/tac_preprocessor.h" // Tac::move

#include <unknwn.h> // IUnknown
#include <guiddef.h>

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
      if( auto unknown = static_cast< IUnknown* >( mT = other.mT ) )
        unknown->AddRef();
    }

    PCom( PCom&& other ) noexcept
    {
      swap( Tac::move(other) );
    }

    ~PCom()
    {
      if( auto unknown = static_cast< IUnknown* >( mT ) )
        unknown->Release();
    }

    // IID_PPV_ARGS == iid(), ppv()
    REFIID iid()          { return __uuidof(*mT); }
    void** ppv()          { return (void**)&mT; }

    // Member functions
    T**    CreateAddress() { return &mT; } // used during creation by a typed api
    T*     Get()           { return mT; }

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
      if( auto unknown = static_cast< IUnknown* >( mT ) )
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
      if( auto unknown = static_cast< IUnknown* >( mT = t ) )
        unknown->AddRef();
    }

    void operator = ( const PCom& other )
    {
      if( auto unknown = static_cast< IUnknown* >( mT ) )
        unknown->Release();

      if( auto unknown = static_cast< IUnknown* >( mT = other.mT ) )
        unknown->AddRef();
    }

    void operator = ( PCom&& other ) noexcept
    {
      swap( Tac::move( other ) );
    }


  private:
    
    T* mT = nullptr;
  };

} // namespace Tac

