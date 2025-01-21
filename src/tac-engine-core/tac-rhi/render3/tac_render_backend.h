#pragma once

#include "tac_render_api.h"

namespace Tac::Render
{
  BufferHandle    AllocBufferHandle();
  PipelineHandle  AllocPipelineHandle();
  ProgramHandle   AllocProgramHandle();
  SamplerHandle   AllocSamplerHandle();
  SwapChainHandle AllocSwapChainHandle();
  TextureHandle   AllocTextureHandle();
  void            FreeHandle( ResourceHandle );
}
