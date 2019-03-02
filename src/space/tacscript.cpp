#include "tacscript.h"
#include "common/tacPreprocessor.h"
//#include "common/imgui.h"


void TacScriptThread::DebugImguiOuter( TacErrors& errors )
{
  //ImGui::PushID( this );
  //OnDestruct( ImGui::PopID() );
  //TacAssert( mName.size() );
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
void TacScriptThread::SetNextKeyDelay( float seconds )
{
  mIsSleeping = true;
  mSecondsToSleep = seconds;
  mSecondsSlept = 0;
}
void TacScriptThread::AddScriptCallback( void* userData, TacScriptCallbackFunction* scriptCallbackFunction )
{
    auto* scriptCallbackData = new TacScriptCallbackData();
  scriptCallbackData->mUserData = userData;
  scriptCallbackData->mScriptCallbackFunction = scriptCallbackFunction;
  mMsgCallbacks.insert( scriptCallbackData );
}
void TacScriptThread::OnMsg( const TacScriptMsg* scriptMsg )
{
  TacVector< TacScriptCallbackData* > toDelete;
  for( TacScriptCallbackData* scriptCallbackData : mMsgCallbacks )
  {
    scriptCallbackData->mScriptCallbackFunction( scriptCallbackData, scriptMsg );
    if( scriptCallbackData->mRequestDeletion )
      toDelete.push_back( scriptCallbackData );
  }
  for( TacScriptCallbackData* deleteMe : toDelete )
  {
    mMsgCallbacks.erase( deleteMe );
    delete deleteMe;
  }
}

TacScriptRoot::~TacScriptRoot()
{
  for( auto child : mChildren )
  {
    delete child;
  }
}
void TacScriptRoot::Update( float seconds, TacErrors& errors )
{
  // threads can add children during the update, so make a copy
  TacVector< TacScriptThread* > childrenToUpdate( mChildren.begin(), mChildren.end() );
  TacVector< TacScriptThread* > childrenToKill;
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
void TacScriptRoot::AddChild( TacScriptThread* child )
{
  TacAssert( child->mName.size() );
  child->mScriptRoot = this;
  mChildren.insert( child );
}
void TacScriptRoot::DebugImgui( TacErrors& errors )
{
  //if( !ImGui::CollapsingHeader( "Script Root" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //for( TacScriptThread* child : mChildren )
  //{
  //  child->DebugImguiOuter( errors );
  //}
}
void TacScriptRoot::OnMsg( const TacScriptMsg* scriptMsg )
{
  TacAssert( scriptMsg->mType.size() );
  for( auto child : mChildren )
    child->OnMsg( scriptMsg );
}
void TacScriptRoot::OnMsg( const TacString& scriptMsgType )
{
  TacScriptMsg scriptMsg;
  scriptMsg.mType = scriptMsgType;
  OnMsg( &scriptMsg );
}
TacScriptThread* TacScriptRoot::GetThread( const TacString& name )
{
  for( auto scriptThread : mChildren )
  {
    if( scriptThread->mName == name )
      return scriptThread;
  }
  return nullptr;
}

