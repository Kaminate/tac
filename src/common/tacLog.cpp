
#include "src/common/tacLog.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/math/tacMath.h"

#include <iostream>

namespace Tac
{

// TODO: fix log unhandled exceptions!!!!!!!!!!!!!!!!!!!
// Q: How to repro?

Log::Log()
{
  int megabytes = 100;
  int kilobytes = 1024 * megabytes;
  int bytes = 1024 * kilobytes;
  Resize( bytes );
}
Log::~Log()
{
  Clear();
}
void Log::Pop()
{
  TAC_ASSERT( mStringCount );
  LogNumber stringByteCount = ReadStringLength( mBeginIndex );
  int freedByteCount = sizeof( LogNumber ) + stringByteCount;
  mBeginIndex = ( mBeginIndex + freedByteCount ) % mByteCount;
  mBytesUsed -= freedByteCount;
  mStringCount--;
}
void Log::Push( StringView stringView )
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
String Log::VisualizeLog()
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
void Log::DebugImgui()
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
LogNumber Log::ReadStringLength( int beginIndex )
{
  LogNumber stringByteCount;
  Read( ( char* )&stringByteCount, beginIndex, sizeof( LogNumber ) );
  return stringByteCount;
}
void Log::ClearData()
{
  if( !mBytes )
    return;
  std::memset( mBytes, 0, mByteCount );
  mBytesUsed = 0;
  mStringCount = 0;
  mBeginIndex = 0;
}
void Log::Clear()
{
  ClearData();
  TAC_DELETE[] mBytes;
  mBytes = nullptr;
  mByteCount = 0;
}
void Log::Resize( int byteCount )
{
  Clear();
  mByteCount = byteCount;
  mBytes = TAC_NEW char[ byteCount ];
  ClearData();
}
void Log::Write( void* src, int bytes )
{
  char* dst = ( char* )mBytes + ( mBeginIndex + mBytesUsed ) % mByteCount;
  mBytesUsed += bytes;

  char* dstEnd = ( char* )dst + bytes;
  char* logEnd = mBytes + mByteCount;
  if( dstEnd > logEnd )
  {
    auto overshoot = ( LogNumber )( dstEnd - logEnd );
    auto writtenByteCount = ( LogNumber )( logEnd - ( char* )dst );
    std::memcpy( dst, src, writtenByteCount );
    dst = mBytes;
    bytes = overshoot;
    src = ( char* )src + writtenByteCount;
  }
  std::memcpy( dst, src, bytes );
}
void Log::Read( char* dst, int beginIndex, int bytes )
{
  char* srcBegin = mBytes + beginIndex;
  char* srcEnd = srcBegin + bytes;

  char* bufferEnd = mBytes + mByteCount;
  if( srcEnd > bufferEnd )
  {
    auto overshootBytes = ( LogNumber )( srcEnd - bufferEnd );
    bytes -= overshootBytes;
    std::memcpy( dst, srcBegin, bytes );
    dst += bytes;
    bytes = overshootBytes;
    srcBegin = mBytes;
  }
  std::memcpy( dst, srcBegin, bytes );
}

}

