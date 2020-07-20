
#pragma once
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include <set>

namespace Tac
{
#define TAC_TIMELINE_KEYFRAME_BEGIN case __COUNTER__: {
#define TAC_TIMELINE_KEYFRAME_END   mLine++; } break;
#define TAC_TIMELINE_KEYFRAME       TAC_TIMELINE_KEYFRAME_END TAC_TIMELINE_KEYFRAME_BEGIN
#define TAC_TIMELINE_BEGIN          switch( mLine ){ case 0: { mLine = __COUNTER__; TAC_TIMELINE_KEYFRAME
#define TAC_TIMELINE_END            mIsComplete = true; TAC_TIMELINE_KEYFRAME_END }

struct Ghost;
struct ScriptRoot;
struct User;
struct ScriptCallbackData;

struct ScriptMsg
{
  String mType;
  void* mData = nullptr;
};

typedef void ScriptCallbackFunction( ScriptCallbackData*, const ScriptMsg* );

struct ScriptCallbackData
{
  void* mUserData = nullptr;
  ScriptCallbackFunction* mScriptCallbackFunction = nullptr;
  bool mRequestDeletion = false;
};

struct ScriptThread
{
  virtual ~ScriptThread() = default;
  virtual void Update( float seconds, Errors& errors ) {}
  void DebugImguiOuter( Errors& errors );
  virtual void DebugImgui( Errors& errors ) {}
  void SetNextKeyDelay( float seconds );
  void OnMsg( const ScriptMsg* scriptMsg );

  void AddScriptCallback( void* userData, ScriptCallbackFunction* scriptCallbackFunction );

  // should this struct have imgui & debug name?

  ScriptRoot* mScriptRoot = nullptr;
  int mLine = 0;
  bool mIsSleeping = false;
  float mSecondsSlept = 0;
  float mSecondsToSleep = 0;
  bool mIsComplete = false;
  String mName;
  std::set< ScriptCallbackData* > mMsgCallbacks;
};

struct ScriptRoot
{
  ~ScriptRoot();
  void AddChild( ScriptThread* child );
  void Update( float seconds, Errors& errors );
  void DebugImgui( Errors& errors );
  void OnMsg( const ScriptMsg* scriptMsg );
  void OnMsg( StringView scriptMsgType );
  ScriptThread* GetThread( StringView name );
  std::set< ScriptThread* > mChildren;
  Ghost* mGhost = nullptr;
};

}

