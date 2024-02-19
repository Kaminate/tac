#include "src/shell/tac_render_state.h" // self-inc

import std; // mutex

namespace Tac
{
  static std::mutex sMutex;

  void GameStateManager::Enqueue( App::IState* state )
  {
    if( !state )
      return;

    TAC_SCOPE_GUARD( std::lock_guard, sMutex );

    auto it = mElements.begin();
    while( mElements.size() > 2 && !it->mUsedCounter )
    {
      TAC_DELETE it->mState;
      it = mElements.erase( it );
    }

    Element element
    {
      .mState = state,
      .mUsedCounter = 0,
    };
    mElements.push_back( element );
  }

  GameStateManager::Pair GameStateManager::Dequeue()
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMutex );

    if( mElements.size() < 2 )
      return {};

    auto it = mElements.rbegin();
    
    App::IState* newState = it->mState;
    int* newCounter = &it->mUsedCounter;

    --it;

    App::IState* oldState = it->mState;
    int* oldCounter = &it->mUsedCounter;

    Pair pair
    {
      .mOldState = newState,
      .mOldUsedCounter = oldCounter,
      .mNewState = newState,
      .mNewUsedCounter = newCounter,
    };

    return pair;
  }

  GameStateManager::Pair::~Pair()
  {
    if( mOldState && mNewState )
    {
      ( *mOldUsedCounter )--;
      ( *mNewUsedCounter )--;
    }
  };
} // namespace Tac
