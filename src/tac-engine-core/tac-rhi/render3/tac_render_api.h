#pragma once

#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct FBHandle
  {

  };


  // i think like a view could be a higher order construct, like in Tac::Space

  struct RenderApi
  {
    struct InitParams
    {
      int mMaxGPUFrameCount = 2;
    };

    static void Init( InitParams, Errors& );
    static int GetMaxGPUFrameCount();

    static FBHandle CreateFB( const void*, v2i );
    static void     ResizeFB( FBHandle, v2i );
    static void     DestroyFB( FBHandle );
  };
}
