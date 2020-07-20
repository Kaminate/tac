
#include "src/space/tacScript.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{


void ScriptThread::DebugImguiOuter( Errors& errors )
{
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
void ScriptThread::SetNextKeyDelay( float seconds )
{
  mIsSleeping = true;
  mSecondsToSleep = seconds;
  mSecondsSlept = 0;
}
void ScriptThread::AddScriptCallback( void* userData, ScriptCallbackFunction* scriptCallbackFunction )
{
    auto* scriptCallbackData = new ScriptCallbackData();
  scriptCallbackData->mUserData = userData;
  scriptCallbackData->mScriptCallbackFunction = scriptCallbackFunction;
  mMsgCallbacks.insert( scriptCallbackData );
}
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
    delete deleteMe;
  }
}

ScriptRoot::~ScriptRoot()
{
  for( auto child : mChildren )
  {
    delete child;
  }
}
void ScriptRoot::Update( float seconds, Errors& errors )
{
  // threads can add children during the update, so make a copy
  Vector< ScriptThread* > childrenToUpdate( mChildren.begin(), mChildren.end() );
  Vector< ScriptThread* > childrenToKill;
  for( auto child : childrenToUpdate )
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
  for( auto child : childrenToKill )
  {
    mChildren.erase( child );
    delete child;
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
  for( auto child : mChildren )
    child->OnMsg( scriptMsg );
}
void ScriptRoot::OnMsg( StringView scriptMsgType )
{
  ScriptMsg scriptMsg;
  scriptMsg.mType = scriptMsgType;
  OnMsg( &scriptMsg );
}
ScriptThread* ScriptRoot::GetThread( StringView name )
{
  for( auto scriptThread : mChildren )
  {
    if( scriptThread->mName == name )
      return scriptThread;
  }
  return nullptr;
}


}

