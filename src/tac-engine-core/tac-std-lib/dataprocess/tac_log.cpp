#include "tac_log.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
#if 0

  // TODO: fix log unhandled exceptions!!!!!!!!!!!!!!!!!!!
  // Q: How to repro?

  LogWindow::LogWindow()
  {
    int megabytes = 100;
    int kilobytes = 1024 * megabytes;
    int bytes = 1024 * kilobytes;
    Resize( bytes );
  }
  LogWindow::~LogWindow()
  {
    Clear();
  }

  void      LogWindow::Pop()
  {
    TAC_ASSERT( mStringCount );
    LogNumber stringByteCount = ReadStringLength( mBeginIndex );
    int freedByteCount = sizeof( LogNumber ) + stringByteCount;
    mBeginIndex = ( mBeginIndex + freedByteCount ) % mByteCount;
    mBytesUsed -= freedByteCount;
    mStringCount--;
  }

  void      LogWindow::Push( const StringView& stringView )
  {
    const char* stringData = stringView.data();
    if( mByteCount < ( int )sizeof( LogNumber ) )
      return;
    LogNumber stringLenMax = mByteCount - sizeof( LogNumber );
    auto stringLenOld = ( LogNumber )stringView.size();
    LogNumber stringLen = Min( stringLenOld, stringLenMax );
    LogNumber requiredByteCount = stringLen + sizeof( LogNumber );
    for( ;; )
    {
      LogNumber remainingByteCount = mByteCount - mBytesUsed;
      if( remainingByteCount >= requiredByteCount )
        break;
      Pop();
    }
    Write( &stringLen, sizeof( LogNumber ) );
    Write( ( void* )stringData, stringLen );
    mStringCount++;
    mScrollToBottom = true;
  }

  String    LogWindow::VisualizeLog()
  {
    int maxVisBytes = 1000;
    if( mByteCount > maxVisBytes )
      return "Log too big to vis, max: " + ToString( maxVisBytes );
    char* bytes = mBytes;
    String result;
    for( int iByte = 0; iByte < mByteCount; ++iByte )
    {
      char c = bytes[ iByte ];
      if( !c )
        c = 'x';
      result += c;
    }
    return result;
  }

  void      LogWindow::DebugImgui()
  {
    //if( !mIsVisible )
    //  return;
    //ImGui::SetNextWindowSize( ImVec2( 520, 600 ), ImGuiCond_FirstUseEver );
    //ImGui::Begin( "Log", &mIsVisible );
    //OnDestruct( ImGui::End() );
    //if( ImGui::CollapsingHeader( "Log Debug" ) )
    //{
    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );
    //  if( ImGui::Button( "Clear" ) )
    //    Clear();
    //  ImGui::InputText( "Add text", mText );
    //  ImGui::SameLine();
    //  if( ImGui::Button( "Push: " ) )
    //    Push( mText );
    //  if( mStringCount && ImGui::Button( "Pop" ) )
    //    Pop();
    //  ImGui::InputInt( "New Size", &mNewSize );
    //  ImGui::SameLine();
    //  if( ImGui::Button( "Resize " ) )
    //    Resize( mNewSize );
    //  ImGui::Text( "Byte count: %i", mByteCount );
    //  ImGui::Text( "Bytes used: %i", mBytesUsed );
    //  ImGui::Text( "String count: %i", mStringCount );
    //  String percentStr = FormatPercentage( ( float )mBytesUsed, ( float )mByteCount );
    //  ImGui::Text( "Percent Used: %s", percentStr.c_str() );
    //  ImGui::Checkbox( "Is visible", &mIsVisible );
    //  if( ImGui::CollapsingHeader( "Debug Test" ) )
    //  {
    //    if( ImGui::Button( "Set DebugTest byte count" ) )
    //      Resize( 1000 );
    //    String debugtext = "ScriptGameClient: ScriptGameClient message: elapsed time is X minutes YY seconds ZZZ miliseconds";
    //    if( ImGui::Button( "Copy DebugTest text" ) )
    //      mText = debugtext;
    //    static bool mDebugTest;
    //    ImGui::Checkbox( "Auto Debug Test", &mDebugTest );
    //    if( mDebugTest )
    //      Push( debugtext );
    //  }
    //  ImGui::Text( '[' + VisualizeLog() + ']' );
    //}
    //if( ImGui::Button( "Clear Data" ) )
    //  ClearData();

    //ImGui::Separator();
    //ImGui::BeginChild( "scrolling", ImVec2( 0, 0 ), false, ImGuiWindowFlags_HorizontalScrollbar );
    //OnDestruct( ImGui::EndChild() );


    //LogNumber beginIndex = mBeginIndex;
    //for( int iLog = 0; iLog < mStringCount; ++iLog )
    //{
    //  LogNumber stringByteCount = ReadStringLength( beginIndex );
    //  beginIndex = ( beginIndex + sizeof( LogNumber ) ) % mByteCount;
    //  String s;
    //  s.resize( stringByteCount );
    //  Read( ( char* )s.data(), beginIndex, stringByteCount );
    //  ImGui::Text( s );
    //  beginIndex = ( beginIndex + stringByteCount ) % mByteCount;
    //}

    //if( mScrollToBottom )
    //{
    //  mScrollToBottom = false;
    //  ImGui::SetScrollHere( 1.0f );
    //}
  }

  LogNumber LogWindow::ReadStringLength( int beginIndex )
  {
    LogNumber stringByteCount;
    Read( ( char* )&stringByteCount, beginIndex, sizeof( LogNumber ) );
    return stringByteCount;
  }

  void      LogWindow::ClearData()
  {
    if( !mBytes )
      return;
    MemSet( mBytes, 0, mByteCount);
    mBytesUsed = 0;
    mStringCount = 0;
    mBeginIndex = 0;
  }

  void      LogWindow::Clear()
  {
    ClearData();
    TAC_DELETE[] mBytes;
    mBytes = nullptr;
    mByteCount = 0;
  }

  void      LogWindow::Resize( int byteCount )
  {
    Clear();
    mByteCount = byteCount;
    mBytes = TAC_NEW char[ byteCount ];
    ClearData();
  }

  void      LogWindow::Write( void* src, int bytes )
  {
    char* dst = ( char* )mBytes + ( mBeginIndex + mBytesUsed ) % mByteCount;
    mBytesUsed += bytes;

    char* dstEnd = ( char* )dst + bytes;
    char* logEnd = mBytes + mByteCount;
    if( dstEnd > logEnd )
    {
      auto overshoot = ( LogNumber )( dstEnd - logEnd );
      auto writtenByteCount = ( LogNumber )( logEnd - ( char* )dst );
      MemCpy( dst, src, writtenByteCount );
      dst = mBytes;
      bytes = overshoot;
      src = ( char* )src + writtenByteCount;
    }
    MemCpy( dst, src, bytes );
  }

  void      LogWindow::Read( char* dst, int beginIndex, int bytes )
  {
    char* srcBegin = mBytes + beginIndex;
    char* srcEnd = srcBegin + bytes;

    char* bufferEnd = mBytes + mByteCount;
    if( srcEnd > bufferEnd )
    {
      auto overshootBytes = ( LogNumber )( srcEnd - bufferEnd );
      bytes -= overshootBytes;
      MemCpy( dst, srcBegin, bytes );
      dst += bytes;
      bytes = overshootBytes;
      srcBegin = mBytes;
    }
    MemCpy( dst, srcBegin, bytes );
  }

#endif

  // -----------------------------------------------------------------------------------------------

  struct File
  {
    void Open(const FileSys::Path& path, std::ios_base::openmode mode )
    {
      mOfs.open( path.u8string().data(), mode );
    }

    void Write( const char* str, int n )
    {
      mOfs.write( str, n );
    }
    
    bool is_open() const { return mOfs.is_open(); }

    std::ofstream    mOfs;
  };

  struct Log
  {
    void Flush();
    void EnsurePath();
    void EnsureOpen();
    void LogMessage( StringView );
    void SetPath( FileSys::Path );

    String           mBuffer;
    FileSys::Path mPath;
    File             mFile;
  };

  // -----------------------------------------------------------------------------------------------

  void Log::EnsureOpen()
  {
    if( mFile.is_open() )
      return;

    EnsurePath();

    const std::ios_base::openmode flags = std::ios::out | std::ios::trunc;
    mFile.Open( mPath, flags );
  }


  void Log::SetPath( FileSys::Path path )
  {
    TAC_ASSERT( !mFile.is_open() );
    mPath = path;
  }

  void Log::EnsurePath()
  {
    if( !mPath.empty() )
      return;

    if( OS::OSOpenDialog )
    {
      Errors dialogErrors;
      FileSys::Path dialogPath = OS::OSOpenDialog( dialogErrors );
      if( dialogErrors.empty() )
      {
        mPath = dialogPath;
        return;
      }
    }

      
    mPath = "tac.log";
  }

  void Log::Flush()
  {
    EnsureOpen();
    mFile.Write( mBuffer.data(), mBuffer.size() );
    mBuffer.clear();

    // Question: Should this close the file and reopen in append mode?
  }

  void Log::LogMessage( StringView sv )
  {
    if( sv.empty() )
      return;

    mBuffer += sv;
    mBuffer += '\n';
    if( mBuffer.size() > 1024 * 1024 * 100 )
      Flush();
  }

  // -----------------------------------------------------------------------------------------------

  static Log sLog;

  // -----------------------------------------------------------------------------------------------

  LogScope::~LogScope()
  {
    sLog.Flush();
  }

  // -----------------------------------------------------------------------------------------------

  void LogApi::LogMessage( const StringView& sv )
  {
    sLog.LogMessage( sv );
  }

  void LogApi::LogStackFrame( const StackFrame& sf )
  {
    sLog.LogMessage( String()
                     + sf.GetFile() + ":"
                     + ToString( sf.GetLine() )
                     + " " + sf.GetFunction() );
  }

  void LogApi::LogFlush()
  {
    sLog.Flush();
  }

  void LogApi::LogSetPath( const FileSys::Path& path )
  {
    sLog.SetPath( path );
  }
} // namespace Tac



