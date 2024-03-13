#pragma once

namespace Tac { struct StringView; }

namespace Tac::Render
{

  struct SubmitRingBuffer
  {
    void                  DebugPrintSubmitAllocInfo() const;
    bool                  IsSubmitAllocated( const void* ) const;
    void*                 SubmitAlloc( int );
    const void*           SubmitAlloc( const void* , const int );
    StringView            SubmitAlloc( const StringView& );

    static const int      sCapacity = 100 * 1024 * 1024;
    char                  mBytes[ sCapacity ];
    int                   mPos;
  };
} // namespace Tac::Render
