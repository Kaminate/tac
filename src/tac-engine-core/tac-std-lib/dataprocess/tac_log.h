// Given a block of memory, treat it as a ring buffer to keep a log
//
// TODO: this whole file is retarded, just use a fuckin Vector<char>
//
// I'm sure the ring buffer can be useful, just not as a log

#pragma once


// this hsould probably be pulled out of tac std lib, and isntead when an asser tis hit,
// it calls an assert callback set by the os.1j
namespace Tac { struct StringView; struct StackFrame; }
namespace Tac::Filesystem { struct Path; }

namespace Tac
{

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

    char*     mBytes          {};
    int       mBeginIndex     {};
    int       mByteCount      {};
    int       mBytesUsed      {};
    bool      mIsVisible      {};
    int       mStringCount    {};
    String    mText           { "" };
    int       mNewSize        { 100 };
    bool      mScrollToBottom {};
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
    void LogSetPath( const Filesystem::Path& );
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

#define TAC_MEDIEVAL_DEBUG Tac::LogApi::LogStackFrame( TAC_STACK_FRAME )

