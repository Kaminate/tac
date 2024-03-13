// Given a block of memory, treat it as a ring buffer to keep a log
//
// TODO: this whole file is retarded, just use a fuckin Vector<char>
//
// I'm sure the ring buffer can be useful, just not as a log

#pragma once

//#include "tac-std-lib/tac_core.h"
//#include "tac-std-lib/string/tac_string.h"
//#include "tac-std-lib/filesystem/tac_filesystem.h"
//#include "tac-std-lib/tac_precompiled_header.h"
//#include "tac-std-lib/tac_ints.h"

//import std; // fstream(ofstream)

// this hsould probably be pulled out of tac std lib, and isntead when an asser tis hit,
// it calls an assert callback set by the os.1j
namespace Tac
{
  struct StringView;
  struct StackFrame;

#if 0
  using LogNumber = u32;

  struct LogWindow
  {
    LogWindow();
    ~LogWindow();
    void      ClearData();
    void      Clear();
    void      Resize( int );
    void      Push( const StringView& );
    void      Pop();
    void      Read( char* dst, int beginIndex, int bytes );
    LogNumber ReadStringLength( int beginIndex );
    void      Write( void*, int );
    void      DebugImgui();
    String    VisualizeLog();

    char*     mBytes = nullptr;
    int       mBeginIndex = 0;
    int       mByteCount = 0;
    int       mBytesUsed = 0;
    bool      mIsVisible = false;
    int       mStringCount = 0;
    String    mText = "";
    int       mNewSize = 100;
    bool      mScrollToBottom = false;
  };
#endif

  // -----------------------------------------------------------------------------------------------

  struct LogScope
  {
    ~LogScope();
  };

  // The purpose of this log here is so that if this program is run outside of a debugger,
  // (for example from PIX), and something goes wrong, there is diagnostics information.
  namespace LogApi
  {
    void LogMessage( const StringView& );
    void LogStackFrame( const StackFrame& );
    void LogFlush();
    void LogSetPath( const StringView& );
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

#define TAC_MEDIEVAL_DEBUG Tac::LogApi::LogStackFrame( TAC_STACK_FRAME )

