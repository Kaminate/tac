#pragma once
#include "common/containers/tacVector.h"
#include "tacPreprocessor.h"
#include <functional>

template< typename... Args >
struct TacEvent
{
  struct Handler
  {
    virtual ~Handler() = default;
    virtual void HandleEvent( Args... eventArgs ) = 0;
  };

  struct Emitter
  {
    void AddCallback( Handler* handler )
    {
      mHandlerSlots.push_back( handler );
    }
    void RemoveCallback( Handler* handler )
    {
      for( int i = 0; i < mHandlerSlots.size(); ++i )
      {
        if( mHandlerSlots[ i ] == handler )
        {
          mHandlerSlots[ i ] = mHandlerSlots.back();
          mHandlerSlots.pop_back();
          return;
        }
      }
      TacInvalidCodePath;
    }
    void EmitEvent( Args... eventArgs )
    {
      for( Handler* handlerSlot : mHandlerSlots )
      {
        handlerSlot->HandleEvent( eventArgs... );
      }
    }
    int size() { return mHandlerSlots.size(); }

  private:
    TacVector< Handler* > mHandlerSlots;
  };
};

struct TacFunctionalHandler : public TacEvent<>::Handler
{
  TacFunctionalHandler( std::function<void()> callback )
  {
    mCallback = callback;
  }
  void HandleEvent() override
  {
    mCallback();
  }
  std::function<void()> mCallback;
};


