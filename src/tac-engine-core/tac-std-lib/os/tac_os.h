#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
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
  extern UTF8Path ( *OSGetApplicationDataPath )( Errors& );

  struct OpenParams
  {
    const UTF8Path* mDefaultFolder {}; // eww wtf is this pointer
  };

  struct SaveParams
  {
    const UTF8Path* mDefaultFolder {}; // eww wtf is this pointer
    const UTF8Path* mSuggestedFilename {}; // eww wtf is this pointer
  };

  extern UTF8Path( *OSSaveDialog )( const SaveParams&, Errors& );
  extern UTF8Path( *OSOpenDialog )( const OpenParams&, Errors& );
  extern void         ( *OSOpenPath )( const UTF8Path&, Errors& );


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

  void CmdLineAddFlag( StringView );
  bool CmdLineIsFlagPresent( StringView );

} // namespace Tac::OS
