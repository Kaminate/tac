#pragma once

#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac::Filesystem { struct Path; }

namespace Tac { struct StringView; struct StackFrame; struct Errors; struct v2; }

namespace Tac
{
  //TAC_DEFINE_HANDLE( SemaphoreHandle );
  //TAC_DEFINE_HANDLE( MutexHandle );

  struct Monitor
  {
    v2i mSize;
  };
}


namespace Tac::OS
{

  extern void        ( *OSDebugBreak )( );

  void                OSDebugAssert( const StringView&, const StackFrame& );
  void                OSDebugPrintLine( const StringView& );
  extern void        ( *OSDebugPopupBox )( const StringView& );

  //                 Gets the path where you can save files to, such as user configs
  extern Filesystem::Path ( *OSGetApplicationDataPath )( Errors& );

  struct SaveParams
  {
    Filesystem::Path* mSuggestedFilename;
  };

  extern Filesystem::Path( *OSSaveDialog )( const SaveParams&, Errors& );
  extern Filesystem::Path( *OSOpenDialog )( Errors& );


  extern Monitor        ( *OSGetPrimaryMonitor )();


  // I don't think this function should exist.
  // If you are debugging, hit a breakpoint, and THEN call this function,
  // you're mouse is likely to have moved.
  // What should happen is that this gets cached during the message pump,
  // and accessed through MouseInput
  //
  //void        OSGetScreenspaceCursorPos( v2&, Errors& );

  // but like.. isnt it weird that Set exists and Get doesnt?
  extern void        ( *OSSetScreenspaceCursorPos )( const v2&, Errors& );

  void                OSThreadSleepSec( float );
  void                OSThreadSleepMsec( int );
  bool                OSAppIsRunning();
  void                OSAppStopRunning();

  extern void*        (*OSGetLoadedDLL)(const StringView& name);
  extern void*        (*OSLoadDLL)(const StringView& path);

  struct ISemaphore
  {
    virtual void DecrementWait() = 0;
    virtual void IncrementPost() = 0;
  };

  // semaphore values cannot go less than 0,
  // so if the semaphore is currently at 0, it needs to wait for
  // someone to increment the semaphore first.
  extern ISemaphore* ( *OSSemaphoreCreate )( );

  //MutexHandle     OSMutexCreate();
  //void            OSMutexLock( MutexHandle );
  //void            OSMutexUnlock( MutexHandle );

} // namespace Tac::OS
