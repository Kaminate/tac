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

  using ScriptCallbackFunction = void (*)( ScriptCallbackData*, const ScriptMsg* );

  struct ScriptCallbackData
  {
    void*                  mUserData               {};
    ScriptCallbackFunction mScriptCallbackFunction {};
    bool                   mRequestDeletion        {};
  };

  // A ScriptThread is based off of CoD's GSC thread.
  // 
  // I think the way it should work is that all game "logic" is written as ScriptThreads.
  // ScriptThreads operate at a higher level than engine systems such as graphics and physics.
  // A ScriptThread itself is not an an engine system, it operates at a higher level, and
  // interacts with the engine through entities/components.
  //
  // So a messaging system exists for script threads to communicate with one another.
  // see example in https://wiki.modme.co/wiki/black_ops_3/guides/Scripting-guide.html
  //
  //    // somewhere in zombie code, when it dies near a player
  //    player notify( "splatter_blood_onscreen" ); 
  //
  //    // a function threaded on a player
  //    function blood_splatter()
  //    {
  //      self endon( "death" ); 
  //      while(1)
  //      {
  //        self waittill( "splatter_blood_on_screen" ); 
  //        iprintlnbold( "splatter blood now" ); 
  //      }
  //    }
  //
  // NEW TAKE:
  //  I think Ghost should inherit ScriptThread and
  //  ...be attached to a persistant gameobject (but we may want the thread to persist after the object dies)
  //  ...be part of a script ecs system?
  //  ...be part of an engine system?
  //
  //  Also, how to separate client script from server script?
  //
  //
  struct ScriptThread
  {
    virtual ~ScriptThread() = default;
    virtual void Update( float seconds, Errors& );
    virtual void DebugImgui( Errors& ) {}
    void DebugImguiOuter( Errors& );
    void SetNextKeyDelay( float seconds );
    void OnMsg( const ScriptMsg* );
    void AddScriptCallback( void* userData, ScriptCallbackFunction );
    void RunForever();

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

#if 1
  struct ScriptRoot
  {
    ~ScriptRoot();
    void AddChild( ScriptThread* child );
    void Update( float seconds, Errors& errors );
    void DebugImgui( Errors& errors );
    void OnMsg( const ScriptMsg* scriptMsg );
    void OnMsg( StringView scriptMsgType );
    auto GetThread( StringView name ) -> ScriptThread*;

    Set< ScriptThread* >      mChildren {};
    //Ghost*                    mGhost    {};
  };
#endif

}

