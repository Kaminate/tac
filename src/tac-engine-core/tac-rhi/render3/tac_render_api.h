#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_stack_frame.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct Handle
  {
    Handle( int );
    int GetIndex() const;
    bool IsValid() const;
  private:
    int mIndex;
  };

#if 0
#define TAC_DEFINE_HANDLE( T ) struct T : public Handle { T( int i = -1 ) : Handle{ i } {} }
  TAC_DEFINE_HANDLE( FBHandle );
  TAC_DEFINE_HANDLE( DynBufHandle );
#undef TAC_DEFINE_HANDLE
#else

  struct FBHandle : public Handle { FBHandle( int i = -1 ) : Handle{ i } {} };
  struct DynBufHandle : public Handle { DynBufHandle( int i = -1 ) : Handle{ i } {} };
#endif


  // i think like a view could be a higher order construct, like in Tac::Space

  struct RenderApi
  {
    struct InitParams
    {
      int mMaxGPUFrameCount = 2;
    };

    struct UpdateDynBufParams
    {
      DynBufHandle mHandle;
      const void*  mSrcBytes;
      int          mSrcByteCount;
      int          mDstByteOffset;
    };

    static void Init( InitParams, Errors& );
    static int GetMaxGPUFrameCount();

    static FBHandle     CreateFB( const void*, v2i, Errors& );
    static void         ResizeFB( FBHandle, v2i );
    static void         DestroyFB( FBHandle );

    static DynBufHandle CreateDynBuf( int, StackFrame, Errors& );
    static void         UpdateDynBuf( UpdateDynBufParams );
    static void         DestroyDynBuf( DynBufHandle );
  };
}
