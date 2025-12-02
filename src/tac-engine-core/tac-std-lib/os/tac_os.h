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
    v2i mPos  {};
    v2i mSize {};
    int mDpi  { 96 }; // win32 default
  };

  struct OpenParams
  {
    UTF8Path mDefaultFolder {};
  };

  struct SaveParams
  {
    UTF8Path mDefaultFolder {};
    UTF8Path mSuggestedFilename {};
  };
}

namespace Tac::OS
{
  extern void      ( *OSDebugBreak )( );
  void                OSDebugAssert( StringView, const StackFrame& );
  void                OSDebugPrintLine( StringView );
  void                OSDebugPrint( StringView );
  extern void      ( *OSDebugPopupBox )( StringView );
  extern auto      ( *OSGetApplicationDataPath )( Errors& )-> UTF8Path; // save files & user configs go here
  extern auto      ( *OSSaveDialog )( const SaveParams&, Errors& ) -> UTF8Path;
  extern auto      ( *OSOpenDialog )( const OpenParams&, Errors& ) -> UTF8Path;
  extern void      ( *OSOpenPath )( const UTF8Path&, Errors& );
  extern auto      ( *OSGetPrimaryMonitor )() -> Monitor;
  extern auto      ( *OSGetMonitorAtPoint )( v2 ) -> Monitor;
  extern auto      ( *OSGetMonitorFromNativeWindowHandle )( const void* ) -> Monitor;
  extern void      ( *OSSetScreenspaceCursorPos )( const v2&, Errors& );
  void                OSThreadSleepSec( float );
  void                OSThreadSleepMsec( int );
  bool                OSAppIsRunning();
  void                OSAppStopRunning();
  extern auto      ( *OSGetLoadedDLL )( StringView name ) -> void*;
  extern auto      ( *OSLoadDLL )( StringView path ) -> void*;
  extern auto      ( *OSGetProcAddress )( void* dll, const StringView path ) -> void*;
  void                CmdLineAddFlag( StringView );
  bool                CmdLineIsFlagPresent( StringView );

} // namespace Tac::OS
