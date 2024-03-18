#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"

namespace Tac::Render
{
  static int sMaxGPUFrameCount; 

  void RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;

    IRenderBackend3::sInstance->Init( errors );
  }

  int RenderApi::GetMaxGPUFrameCount()
  {
    return sMaxGPUFrameCount;
  }
}
