// Given a block of memory, treat it as a ring buffer to keep a log
//
// TODO: this whole file is retarded, just use a fuckin Vector<char>
//
// I'm sure the ring buffer can be useful, just not as a log

#pragma once
#include "src/common/string/tacString.h"
//#include "src/common/ErrorHandling.h"
#include <cstdint>

namespace Tac
{
  typedef uint32_t LogNumber;
  struct Log
  {
    Log();
    ~Log();
    void      ClearData();
    void      Clear();
    void      Resize( int );
    void      Push( StringView s );
    void      Pop();
    void      Read( char* dst, int beginIndex, int bytes );
    LogNumber ReadStringLength( int beginIndex );
    void      Write( void* src, int bytes );
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
}

