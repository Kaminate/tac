#if 0
#include "tac_render_state.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/mutex/tac_mutex.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

//import std; // mutex

namespace Tac
{
  static Mutex sMutex; // protects mElements
  static bool sVerbose;

  static void RemoveElements()
  {
  }

  void GameStateManager::CleanUnneeded()
  {
    if( mElements.size() < 2 )
      return;

    if( sVerbose )
    {
      OS::OSDebugPrintLine( String() + "element count: " + ToString( mElements.size() ) );

      int i{};
      for( auto it { mElements.begin() }; it != mElements.end(); ++it )
      {
        OS::OSDebugPrintLine
        (
          String()
          + ToString( i ) + ", "
          + "node addr: " + ToString( it.mNode ) + ", "
          + "frame idx: " + ToString( it.mNode->mT.mState->mFrameIndex ) + ", "
          + "used counter: " + ToString( it.mNode->mT.mUsedCounter )
        );
        it.mNode;

        ++i;
      }
    }

    auto it { mElements.rbegin() };


    // Don't remove most recent 2 elements, so a pair can be dequeueud
    --it;
    --it;

    // remove all the unused elements
    while( it != mElements.end() )
    {
      auto elementIt { it };
      --it;

      if( Element& element { *elementIt }; !element.mUsedCounter )
      {
        TAC_DELETE element.mState;
        mElements.erase( elementIt );
      }
    }
  }

  void GameStateManager::Enqueue( App::IState* state )
  {
    if( !state )
      return;

    TAC_SCOPE_GUARD( LockGuard, sMutex );

    CleanUnneeded();

    Element element
    {
      .mState { state },
      .mUsedCounter {},
    };
    mElements.push_back( element );
    TAC_ASSERT_MSG( mElements.size() < 10, "sanity check" );
  }

  GameStateManager::Pair GameStateManager::Dequeue()
  {
    TAC_SCOPE_GUARD( LockGuard, sMutex );

    if( const int n { mElements.size() }; n < 2 )
      return {};

    //Timestamp ts = Timestep::GetElapsedTime();
    //FrameIndex fi = ShellGetFrameIndex();
    //float t = ShellGetInterpolationPercent();

    auto it { mElements.rbegin() };
    //while( it != mElements.end() )
    //{
    //  Element& element = *it--;
    //  if( element.mState->mTimestamp 
    //}

    Element& newStateElement { *it-- };
    Element& oldStateElement { *it-- };

    TAC_ASSERT( oldStateElement.mState != newStateElement.mState );
    oldStateElement.mUsedCounter++;
    newStateElement.mUsedCounter++;

    return Pair 
    {
      .mOldState       { oldStateElement.mState },
      .mOldUsedCounter { &oldStateElement.mUsedCounter },
      .mNewState       { newStateElement.mState },
      .mNewUsedCounter { &newStateElement.mUsedCounter },
    };
  }

  GameStateManager::Pair::~Pair()
  {
    if( !IsValid() )
      return;

    int& n1 { *mOldUsedCounter };
    int& n2 { *mNewUsedCounter };
    n1--;
    n2--;

    TAC_ASSERT( n1 >= 0 && n1 <= 2 );
    TAC_ASSERT( n2 >= 0 && n2 <= 2 );
  };
} // namespace Tac
#endif
