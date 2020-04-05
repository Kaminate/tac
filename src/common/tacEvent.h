#pragma once
#include "src/common/containers/tacVector.h"
#include <functional>
namespace Tac
{


template< typename... Args >
struct Event
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
    void clear()
    {
      mFunctionalHandlerSlots.clear();
    }
  private:
    Vector< std::function< void( Args... ) > > mFunctionalHandlerSlots;
  };
};


}
