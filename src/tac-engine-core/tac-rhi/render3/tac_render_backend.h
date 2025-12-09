#pragma once

#include "tac_render_api.h"

namespace Tac::Render
{
  auto AllocBufferHandle() -> BufferHandle;
  auto AllocPipelineHandle() -> PipelineHandle;
  auto AllocProgramHandle() -> ProgramHandle;
  auto AllocSamplerHandle() -> SamplerHandle;
  auto AllocSwapChainHandle() -> SwapChainHandle;
  auto AllocTextureHandle() -> TextureHandle;
  void FreeHandle( ResourceHandle );
}
