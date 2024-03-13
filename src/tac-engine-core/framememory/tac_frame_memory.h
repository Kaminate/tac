#pragma once

//#include "tac-std-lib/tac_core.h"
//#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  struct StringView;

  void        FrameMemoryInitThreadAllocator( int byteCount );
  void*       FrameMemoryAllocate( int );
  StringView  FrameMemoryCopy( const StringView& );
  StringView  FrameMemoryCopy( const void*, int n);
  bool        FrameMemoryIsAllocated( const void* );


} // namespace Tac

