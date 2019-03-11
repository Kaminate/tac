#pragma once
#include "common/containers/tacVector.h"
#include <functional>

template< typename... Args >
struct TacEvent
{
  struct Emitter
  {
    void AddCallbackFunctional( std::function< void( Args... ) > callback )
    {
      mFunctionalHandlerSlots.push_back( callback );
    }
    void EmitEvent( Args... eventArgs )
    {
      for( const auto& callback : mFunctionalHandlerSlots )
      {
        callback( eventArgs... );
      }
    }
    int size()
    {
      return mFunctionalHandlerSlots.size();
    }
  private:
    TacVector< std::function< void( Args... ) > > mFunctionalHandlerSlots;
  };
};


