// Given a block of memory, treat it as a ring buffer to keep a log
//
// TODO: this whole file is retarded, just use a fuckin TacVector<char>
//
// I'm sure the ring buffer can be useful, just not as a log

#pragma once
#include "common/tacString.h"
//#include "common/tacErrorHandling.h"
#include <cstdint>

typedef uint32_t TacLogNumber;
struct TacLog
{
  TacLog();
  ~TacLog();
  void ClearData();
  void Clear();
  void Resize( int );
  void Push( const TacString& s );
  void Push( const char* );
  void Pop();
  void Read( char* dst, int beginIndex, int bytes );
  TacLogNumber ReadStringLength( int beginIndex );
  void Write( void* src, int bytes );
  void DebugImgui();
  TacString VisualizeLog();

  char* mBytes = nullptr;
  int mBeginIndex = 0;
  int mByteCount = 0;
  int mBytesUsed = 0;
  bool mIsVisible = false;
  int mStringCount = 0;
  TacString mText = "";
  int mNewSize = 100;
  bool mScrollToBottom = false;
};
