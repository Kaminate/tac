#pragma once

#include "tac_render_api.h"
#include "tac-rhi/identifier/tac_id_collection.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct IContextBackend
  {
    virtual void SetViewport( v2i ) {}
    virtual void SetScissor( v2i ) {}
    virtual void SetRenderTarget( FBHandle ) {}
    virtual void Execute( Errors& ) {}
    virtual void ExecuteSynchronously( Errors& ) {}
    virtual void DebugEvent( StringView ) {}
    virtual void DebugMarker( StringView ) {}
    virtual void Retire() = 0;
  };

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
