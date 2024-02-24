#include "src/shell/tac_render_state.h" // self-inc

import std; // mutex

namespace Tac
{
  static std::mutex sMutex; // protects mElements

  void GameStateManager::Enqueue( App::IState* state )
  {
    if( !state )
      return;

    TAC_SCOPE_GUARD( std::lock_guard, sMutex );

    // leave 2 elements, so something can be dequeued
    int n = mElements.size();
    while( n > 2 )
    {
      Element& element = mElements.front();
      if( element.mUsedCounter )
        break;

      TAC_DELETE element.mState;
      mElements.pop_front();
      n--;
    }

    Element element
    {
      .mState = state,
      .mUsedCounter = 0,
    };
    mElements.push_back( element );
    TAC_ASSERT_MSG( mElements.size() < 10, "sanity check" );
  }

  GameStateManager::Pair GameStateManager::Dequeue()
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );

    if( const int n = mElements.size(); n < 2 )
      return {};

    auto it = mElements.rbegin();
    Element& newStateElement = *it--;
    Element& oldStateElement = *it--;

    TAC_ASSERT( oldStateElement.mState != newStateElement.mState );
    oldStateElement.mUsedCounter++;
    newStateElement.mUsedCounter++;

    return Pair 
    {
      .mOldState = oldStateElement.mState,
      .mOldUsedCounter = &oldStateElement.mUsedCounter,
      .mNewState = newStateElement.mState,
      .mNewUsedCounter = &newStateElement.mUsedCounter,
    };
  }

  GameStateManager::Pair::~Pair()
  {
    if( !IsValid() )
      return;

    int& n1 = *mOldUsedCounter;
    int& n2 = *mNewUsedCounter;
    n1--;
    n2--;

    TAC_ASSERT( n1 >= 0 && n1 <= 2 );
    TAC_ASSERT( n2 >= 0 && n2 <= 2 );
  };
} // namespace Tac
