#pragma once

#include "tac_render_api.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{

  BufferHandle    AllocBufferHandle();
  PipelineHandle  AllocPipelineHandle();
  ProgramHandle   AllocProgramHandle();
  SamplerHandle   AllocSamplerHandle();
  SwapChainHandle AllocSwapChainHandle();
  TextureHandle   AllocTextureHandle();

  void            FreeHandle( BufferHandle );
  void            FreeHandle( PipelineHandle );
  void            FreeHandle( ProgramHandle );
  void            FreeHandle( SamplerHandle );
  void            FreeHandle( SwapChainHandle );
  void            FreeHandle( TextureHandle );
}
