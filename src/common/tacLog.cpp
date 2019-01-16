#include "tacLog.h"
#include "tacPreprocessor.h"
#include "tacUtility.h"
#include "tacAlgorithm.h"
#include "common/math/tacMath.h"

#include "imgui.h"

#include <iostream>


// TODO: fix log unhandled exceptions!!!!!!!!!!!!!!!!!!!
// Q: How to repro?

TacLog::TacLog()
{
  int megabytes = 100;
  int kilobytes = 1024 * megabytes;
  int bytes = 1024 * kilobytes;
  Resize( bytes );
}
TacLog::~TacLog()
{
  Clear();
}
void TacLog::Pop()
{
  TacAssert( mStringCount );
  TacLogNumber stringByteCount = ReadStringLength( mBeginIndex );
  int freedByteCount = sizeof( TacLogNumber ) + stringByteCount;
  mBeginIndex = ( mBeginIndex + freedByteCount ) % mByteCount;
  mBytesUsed -= freedByteCount;
  mStringCount--;
}
void TacLog::Push( const char* stringData )
{
  if( mByteCount < ( int )sizeof( TacLogNumber ) )
    return;
  TacLogNumber stringLenMax = mByteCount - sizeof( TacLogNumber );
  auto stringLenOld = ( TacLogNumber )strlen( stringData );
  TacLogNumber stringLen = TacMin( stringLenOld, stringLenMax );
  TacLogNumber requiredByteCount = stringLen + sizeof( TacLogNumber );
  for( ;; )
  {
    TacLogNumber remainingByteCount = mByteCount - mBytesUsed;
    if( remainingByteCount >= requiredByteCount )
      break;
    Pop();
  }
  Write( &stringLen, sizeof( TacLogNumber ) );
  Write( ( void* )stringData, stringLen );
  mStringCount++;
  mScrollToBottom = true;
}
void TacLog::Push( const TacString& s )
{
  Push( s.c_str() );
}
TacString TacLog::VisualizeLog()
{
  int maxVisBytes = 1000;
  if( mByteCount > maxVisBytes )
    return "Log too big to vis, max: " + TacToString( maxVisBytes );
  char* bytes = mBytes;
  TacString result;
  for( int iByte = 0; iByte < mByteCount; ++iByte )
  {
    char c = bytes[ iByte ];
    if( !c )
      c = 'x';
    result += c;
  }
  return result;
}
void TacLog::DebugImgui()
{
  if( !mIsVisible )
    return;
  ImGui::SetNextWindowSize( ImVec2( 520, 600 ), ImGuiCond_FirstUseEver );
  ImGui::Begin( "Log", &mIsVisible );
  OnDestruct( ImGui::End() );
  if( ImGui::CollapsingHeader( "Log Debug" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    if( ImGui::Button( "Clear" ) )
      Clear();
    ImGui::InputText( "Add text", mText );
    ImGui::SameLine();
    if( ImGui::Button( "Push: " ) )
      Push( mText );
    if( mStringCount && ImGui::Button( "Pop" ) )
      Pop();
    ImGui::InputInt( "New Size", &mNewSize );
    ImGui::SameLine();
    if( ImGui::Button( "Resize " ) )
      Resize( mNewSize );
    ImGui::Text( "Byte count: %i", mByteCount );
    ImGui::Text( "Bytes used: %i", mBytesUsed );
    ImGui::Text( "String count: %i", mStringCount );
    TacString percentStr = TacFormatPercentage( ( float )mBytesUsed, ( float )mByteCount );
    ImGui::Text( "Percent Used: %s", percentStr.c_str() );
    ImGui::Checkbox( "Is visible", &mIsVisible );
    if( ImGui::CollapsingHeader( "Debug Test" ) )
    {
      if( ImGui::Button( "Set DebugTest byte count" ) )
        Resize( 1000 );
      TacString debugtext = "TacScriptGameClient: TacScriptGameClient message: elapsed time is X minutes YY seconds ZZZ miliseconds";
      if( ImGui::Button( "Copy DebugTest text" ) )
        mText = debugtext;
      static bool mDebugTest;
      ImGui::Checkbox( "Auto Debug Test", &mDebugTest );
      if( mDebugTest )
        Push( debugtext );
    }
    ImGui::Text( '[' + VisualizeLog() + ']' );
  }
  if( ImGui::Button( "Clear Data" ) )
    ClearData();

  ImGui::Separator();
  ImGui::BeginChild( "scrolling", ImVec2( 0, 0 ), false, ImGuiWindowFlags_HorizontalScrollbar );
  OnDestruct( ImGui::EndChild() );


  TacLogNumber beginIndex = mBeginIndex;
  for( int iLog = 0; iLog < mStringCount; ++iLog )
  {
    TacLogNumber stringByteCount = ReadStringLength( beginIndex );
    beginIndex = ( beginIndex + sizeof( TacLogNumber ) ) % mByteCount;
    TacString s;
    s.resize( stringByteCount );
    Read( ( char* )s.data(), beginIndex, stringByteCount );
    ImGui::Text( s );
    beginIndex = ( beginIndex + stringByteCount ) % mByteCount;
  }

  if( mScrollToBottom )
  {
    mScrollToBottom = false;
    ImGui::SetScrollHere( 1.0f );
  }
}
TacLogNumber TacLog::ReadStringLength( int beginIndex )
{
  TacLogNumber stringByteCount;
  Read( ( char* )&stringByteCount, beginIndex, sizeof( TacLogNumber ) );
  return stringByteCount;
}
void TacLog::ClearData()
{
  if( !mBytes )
    return;
  std::memset( mBytes, 0, mByteCount );
  mBytesUsed = 0;
  mStringCount = 0;
  mBeginIndex = 0;
}
void TacLog::Clear()
{
  ClearData();
  delete mBytes;
  mBytes = nullptr;
  mByteCount = 0;
}
void TacLog::Resize( int byteCount )
{
  Clear();
  mByteCount = byteCount;
  mBytes = new char[ byteCount ];
  ClearData();
}
void TacLog::Write( void* src, int bytes )
{
  char* dst = ( char* )mBytes + ( mBeginIndex + mBytesUsed ) % mByteCount;
  mBytesUsed += bytes;

  char* dstEnd = ( char* )dst + bytes;
  char* logEnd = mBytes + mByteCount;
  if( dstEnd > logEnd )
  {
    auto overshoot = ( TacLogNumber )( dstEnd - logEnd );
    auto writtenByteCount = ( TacLogNumber )( logEnd - ( char* )dst );
    std::memcpy( dst, src, writtenByteCount );
    dst = mBytes;
    bytes = overshoot;
    src = ( char* )src + writtenByteCount;
  }
  std::memcpy( dst, src, bytes );
}
void TacLog::Read( char* dst, int beginIndex, int bytes )
{
  char* srcBegin = mBytes + beginIndex;
  char* srcEnd = srcBegin + bytes;

  char* bufferEnd = mBytes + mByteCount;
  if( srcEnd > bufferEnd )
  {
    auto overshootBytes = ( TacLogNumber )( srcEnd - bufferEnd );
    bytes -= overshootBytes;
    std::memcpy( dst, srcBegin, bytes );
    dst += bytes;
    bytes = overshootBytes;
    srcBegin = mBytes;
  }
  std::memcpy( dst, srcBegin, bytes );
}
