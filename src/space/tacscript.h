#pragma once
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include <set>

#define TAC_TIMELINE_KEYFRAME_BEGIN case __COUNTER__: {
#define TAC_TIMELINE_KEYFRAME_END   mLine++; } break;
#define TAC_TIMELINE_KEYFRAME       TAC_TIMELINE_KEYFRAME_END TAC_TIMELINE_KEYFRAME_BEGIN
#define TAC_TIMELINE_BEGIN          switch( mLine ){ case 0: { mLine = __COUNTER__; TAC_TIMELINE_KEYFRAME
#define TAC_TIMELINE_END            mIsComplete = true; TAC_TIMELINE_KEYFRAME_END }

struct TacGhost;
struct TacScriptRoot;
struct TacUser;
struct TacScriptCallbackData;

struct TacScriptMsg
{
  TacString mType;
  void* mData = nullptr;
};

typedef void TacScriptCallbackFunction( TacScriptCallbackData*, const TacScriptMsg* );

struct TacScriptCallbackData
{
  void* mUserData = nullptr;
  TacScriptCallbackFunction* mScriptCallbackFunction = nullptr;
  bool mRequestDeletion = false;
};

struct TacScriptThread
{
  virtual ~TacScriptThread() = default;
  virtual void Update( float seconds, TacErrors& errors ) {}
  void DebugImguiOuter( TacErrors& errors );
  virtual void DebugImgui( TacErrors& errors ) {}
  void SetNextKeyDelay( float seconds );
  void OnMsg( const TacScriptMsg* scriptMsg );

  void AddScriptCallback( void* userData, TacScriptCallbackFunction* scriptCallbackFunction );

  // should this struct have imgui & debug name?

  TacScriptRoot* mScriptRoot = nullptr;
  int mLine = 0;
  bool mIsSleeping = false;
  float mSecondsSlept = 0;
  float mSecondsToSleep = 0;
  bool mIsComplete = false;
  TacString mName;
  std::set< TacScriptCallbackData* > mMsgCallbacks;
};

struct TacScriptRoot
{
  ~TacScriptRoot();
  void AddChild( TacScriptThread* child );
  void Update( float seconds, TacErrors& errors );
  void DebugImgui( TacErrors& errors );
  void OnMsg( const TacScriptMsg* scriptMsg );
  void OnMsg( const TacString& scriptMsgType );
  TacScriptThread* GetThread( const TacString& name );
  std::set< TacScriptThread* > mChildren;
  TacGhost* mGhost = nullptr;
};