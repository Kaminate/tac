#pragma once

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct RenderApi
  {
    struct InitParams
    {
      int mMaxGPUFrameCount = 2;
    };

    static void Init( InitParams, Errors& );
    static int GetMaxGPUFrameCount();
  };
}
