#pragma once

#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac::FileSys { struct Path; }

namespace Tac
{
  struct StringView;
  struct StackFrame;
  struct Errors;
  struct v2;

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
  void                OSDebugPrint( const StringView& );
  extern void        ( *OSDebugPopupBox )( const StringView& );

  //                 Gets the path where you can save files to, such as user configs
  extern FileSys::Path ( *OSGetApplicationDataPath )( Errors& );

  struct OpenParams
  {
    const FileSys::Path* mDefaultFolder {}; // eww wtf is this pointer
  };

  struct SaveParams
  {
    const FileSys::Path* mDefaultFolder {}; // eww wtf is this pointer
    const FileSys::Path* mSuggestedFilename {}; // eww wtf is this pointer
  };

  extern FileSys::Path( *OSSaveDialog )( const SaveParams&, Errors& );
  extern FileSys::Path( *OSOpenDialog )( const OpenParams&, Errors& );
  extern void         ( *OSOpenPath )( const FileSys::Path&, Errors& );


  extern Monitor      ( *OSGetPrimaryMonitor )();


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

  extern void*        ( *OSGetLoadedDLL )( const StringView& name );
  extern void*        ( *OSLoadDLL )( const StringView& path );
  extern void*        ( *OSGetProcAddress )( void* dll, const StringView& path );

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

  void CmdLineAddFlag( StringView );
  bool CmdLineIsFlagPresent( StringView );

} // namespace Tac::OS
