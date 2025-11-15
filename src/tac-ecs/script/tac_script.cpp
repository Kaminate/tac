
#include "tac-ecs/script/tac_script.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/containers/tac_frame_vector.h"

namespace Tac
{


  void ScriptThread::Update( float, Errors& )
  {

  }

  void ScriptThread::DebugImguiOuter( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //ImGui::PushID( this );
    //OnDestruct( ImGui::PopID() );
    //Assert( mName.size() );
    //if( !ImGui::CollapsingHeader( mName.c_str() ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::DragInt( "mLine", &mLine );
    //if( ImGui::Button( "Replay" ) )
    //  mLine = 0;
    //if( ImGui::Button( "Complete" ) )
    //  mIsComplete = true;
    //DebugImgui( errors );
  }

  void ScriptThread::Sleep( TimeDuration time )
  {
    mIsSleeping = true;
    mSecondsToSleep = time.mSeconds;
    mSecondsSlept = 0;
  }

  void ScriptThread::AddScriptCallback( void* userData, ScriptCallbackFunction fn )
  {
    auto* scriptCallbackData { TAC_NEW ScriptCallbackData };
    scriptCallbackData->mUserData = userData;
    scriptCallbackData->mScriptCallbackFunction = fn;
    mMsgCallbacks.insert( scriptCallbackData );
  }

  void ScriptThread::RunForever() { mRunForever = true; }

  void ScriptThread::OnMsg( const ScriptMsg* scriptMsg )
  {
    Vector< ScriptCallbackData* > toDelete;
    for( ScriptCallbackData* scriptCallbackData : mMsgCallbacks )
    {
      scriptCallbackData->mScriptCallbackFunction( scriptCallbackData, scriptMsg );
      if( scriptCallbackData->mRequestDeletion )
        toDelete.push_back( scriptCallbackData );
    }
    for( ScriptCallbackData* deleteMe : toDelete )
    {
      mMsgCallbacks.erase( deleteMe );
      TAC_DELETE deleteMe;
    }
  }

#if 1
  ScriptRoot::~ScriptRoot()
  {
    for( ScriptThread* child : mChildren )
    {
      TAC_DELETE child;
    }
  }

  void ScriptRoot::Update( float seconds, Errors& errors )
  {
    // threads can add children during the update, so make a copy
    FrameMemoryVector< ScriptThread* > childrenToUpdate( mChildren.begin(), mChildren.end() );
    FrameMemoryVector< ScriptThread* > childrenToKill;
    for( ScriptThread* child : childrenToUpdate )
    {
      // should we check for complete? ( ie: child A sends a message that completes child B )
      if( child->mIsSleeping )
      {
        child->mSecondsSlept += seconds;
        if( child->mSecondsSlept > child->mSecondsToSleep )
          child->mIsSleeping = false;

        continue;
      }

      child->Update( seconds, errors );
      if( child->mIsComplete )
        childrenToKill.push_back( child );
    }

    for( ScriptThread* child : childrenToKill )
    {
      mChildren.erase( child );
      TAC_DELETE child;
    }
  }

  void ScriptRoot::AddChild( ScriptThread* child )
  {
    TAC_ASSERT( child->mName.size() );
    child->mScriptRoot = this;
    mChildren.insert( child );
  }

  void ScriptRoot::DebugImgui( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //if( !ImGui::CollapsingHeader( "Script Root" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //for( ScriptThread* child : mChildren )
    //{
    //  child->DebugImguiOuter( errors );
    //}
  }

  void ScriptRoot::OnMsg( const ScriptMsg* scriptMsg )
  {
    TAC_ASSERT( scriptMsg->mType.size() );
    for( ScriptThread* child : mChildren )
      child->OnMsg( scriptMsg );
  }

  void ScriptRoot::OnMsg( StringView scriptMsgType )
  {
    ScriptMsg scriptMsg;
    scriptMsg.mType = scriptMsgType;
    OnMsg( &scriptMsg );
  }

  auto ScriptRoot::GetThread( StringView name ) -> ScriptThread*
  {
    for( ScriptThread* scriptThread : mChildren )
      if( ( StringView )scriptThread->mName == name )
        return scriptThread;
    return nullptr;
  }
#endif


} // namespace Tac

