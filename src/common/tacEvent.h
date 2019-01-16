#pragma once
#include "common/containers/tacVector.h"
#include "tacPreprocessor.h"

template< typename... Args >
struct TacEvent
{
  struct Handler
  {
    virtual ~Handler() = default;
    virtual void HandleEvent( Args... eventArgs ) = 0;
  };

  //template< typename T >
  //struct THandler : public Handler
  //{
  //  THandler( T t ) : mT( t ) {};
  //  void HandleEvent( Args... eventArgs ) override
  //  {
  //    mT( eventArgs... );
  //  }
  //  T mT;
  //};

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

  private:
    TacVector< Handler* > mHandlerSlots;
  };
};


