#pragma once

#include "tac_render_api.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{

  FBHandle        AllocFBHandle();
  PipelineHandle  AllocPipelineHandle();
  ProgramHandle   AllocProgramHandle();
  BufferHandle    AllocBufferHandle();
  TextureHandle   AllocTextureHandle();
  void            FreeHandle( FBHandle );
  void            FreeHandle( PipelineHandle );
  void            FreeHandle( ProgramHandle );
  void            FreeHandle( BufferHandle );
  void            FreeHandle( TextureHandle );
}
