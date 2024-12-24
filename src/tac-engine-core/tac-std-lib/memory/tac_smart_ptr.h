#pragma once

#include "tac-std-lib/memory/tac_memory.h" // TAC_NEW/DELETE
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

namespace Tac
{
  template< typename T >
  struct SmartPtr
  {
    SmartPtr() = default;
    SmartPtr( T* t ) : mT{ t }           {}
    SmartPtr( SmartPtr&& s )             { SwapWith( ( SmartPtr&& )s ); }

    SmartPtr( const SmartPtr& s )
    {
      if( !s )
        return;

      s.Increment();
      mRefCounter = s.mRefCounter;
      mT = s.mT;
    }

    template< class ... Args >
    SmartPtr( Args ... args ) : mT{ TAC_NEW T{ args... } } {}

    template< typename U >
    SmartPtr( U* u ) : mT{ static_cast< T* >( u ) } {}

    ~SmartPtr()                          { Decrement(); }

    void     clear()
    {
      Decrement();
      mT = nullptr;
      mRefCounter = nullptr;
    }
    T*       Get()                       { return mT; }
    operator T* ()                       { return mT; }
    void     operator = ( T* t )         { mT = t; }
    void     operator = ( SmartPtr&& s ) { SwapWith( ( SmartPtr&& )s ); }
    void     operator = ( const SmartPtr& s )
    {
      if( !s )
        return;

      clear();
      s.Increment();
      mRefCounter = s.mRefCounter;
      mT = s.mT;
    }
    T*       operator ->()               { return mT; }
    const T* operator ->() const         { return mT; }
    operator bool() const                { return mT; }

  private:

    void SwapWith( SmartPtr&& other )
    {
      Swap( mRefCounter, other.mRefCounter );
      Swap( mT, other.mT );
    }

    // Feels wrong... Decrement/Increment() are const, mRefCounter/mT are mutable

    void Decrement() const
    {
      if( mRefCounter )
      {
        int& n { *mRefCounter };
        --n;
        if( !n )
        {
          TAC_DELETE mT;
          TAC_DELETE mRefCounter;
          mT = nullptr;
          mRefCounter = nullptr;
        }
      }
      else if( mT )
      {
        TAC_DELETE mT;
        mT = nullptr;
      }
    }

    void Increment() const
    {
      if( !mRefCounter )
        mRefCounter = TAC_NEW int{ mT ? 1 : 0 };

      ( *mRefCounter )++;
    }

    mutable int* mRefCounter {};
    mutable T*   mT          {};
  };
} // namespace Tac

