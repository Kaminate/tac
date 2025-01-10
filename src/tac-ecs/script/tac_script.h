#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_set.h"

namespace Tac
{

#define TAC_TIMELINE_KEYFRAME_BEGIN case __COUNTER__: {
#define TAC_TIMELINE_KEYFRAME_END   mLine++; } break;
#define TAC_TIMELINE_KEYFRAME       TAC_TIMELINE_KEYFRAME_END TAC_TIMELINE_KEYFRAME_BEGIN
#define TAC_TIMELINE_BEGIN          switch( mLine ){ case 0: { mLine = __COUNTER__; TAC_TIMELINE_KEYFRAME
#define TAC_TIMELINE_END            if( !mRunForever ) mIsComplete = true; TAC_TIMELINE_KEYFRAME_END }

  struct Ghost;
  struct ScriptRoot;
  struct User;
  struct ScriptCallbackData;

  struct ScriptMsg
  {
    String mType {};
    void*  mData {};
  };

  typedef void ScriptCallbackFunction( ScriptCallbackData*, const ScriptMsg* );

  struct ScriptCallbackData
  {
    void*                   mUserData               {};
    ScriptCallbackFunction* mScriptCallbackFunction {};
    bool                    mRequestDeletion        {};
  };

  struct ScriptThread
  {
    virtual ~ScriptThread() = default;
    virtual void                    Update( float seconds, Errors& );
    void                            DebugImguiOuter( Errors& );
    virtual void                    DebugImgui( Errors& ) {}
    void                            SetNextKeyDelay( float seconds );
    void                            OnMsg( const ScriptMsg* );
    void                            AddScriptCallback( void* userData, ScriptCallbackFunction* );
    void                            RunForever();

    ScriptRoot*                     mScriptRoot     {};
    int                             mLine           {};
    bool                            mIsSleeping     {};
    float                           mSecondsSlept   {};
    float                           mSecondsToSleep {};
    bool                            mIsComplete     {};
    bool                            mRunForever     {};
    String                          mName           {};
    Set< ScriptCallbackData* >      mMsgCallbacks   {};
  };

  struct ScriptRoot
  {
    ~ScriptRoot();
    void                      AddChild( ScriptThread* child );
    void                      Update( float seconds, Errors& errors );
    void                      DebugImgui( Errors& errors );
    void                      OnMsg( const ScriptMsg* scriptMsg );
    void                      OnMsg( StringView scriptMsgType );
    ScriptThread*             GetThread( StringView name );

    Set< ScriptThread* >      mChildren {};
    Ghost*                    mGhost    {};
  };

}

