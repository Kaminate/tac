#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"

namespace Tac::Render
{
  static int sMaxGPUFrameCount; 

  static IRenderBackend3* sBackend;

  void RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sBackend = IRenderBackend3::sInstance;
    sBackend->Init( errors );
  }

  int RenderApi::GetMaxGPUFrameCount()
  {
    return sMaxGPUFrameCount;
  }

  FBHandle RenderApi::CreateFB( const void* nwh, v2i size )
  {
    return sBackend->CreateFB( nwh, size );
  }

  void     RenderApi::ResizeFB( FBHandle h, v2i size )
  {
    sBackend->ResizeFB( h, size );
  }

  void     RenderApi::DestroyFB( FBHandle h )
  {
    sBackend->DestroyFB( h );
  }

}
