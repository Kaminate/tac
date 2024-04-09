#pragma once

#include "tac_render_api.h"

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
    virtual void Retire() = 0;
  };

  struct IRenderBackend3
  {
    IRenderBackend3();
    virtual void Init( Errors& ) {};

    virtual void    CreateRenderPipeline( PipelineHandle, PipelineParams, Errors& ) {}
    virtual void    DestroyRenderPipeline( PipelineHandle ) {}

    virtual void    CreateProgram( ProgramHandle, ProgramParams, Errors& ) {}
    virtual void    DestroyProgram( ProgramHandle ) {}

    virtual void    CreateFB( FBHandle, FrameBufferParams, Errors& ) {}
    virtual void    ResizeFB( FBHandle, v2i ) {}
    virtual TexFmt  GetFBFmt( FBHandle ) { return TexFmt::kUnknown; }
    virtual void    DestroyFB( FBHandle ) {}

    virtual void    CreateDynBuf( DynBufHandle, int, StackFrame, Errors& ) {}
    virtual void    UpdateDynBuf( RenderApi::UpdateDynBufParams ) {}
    virtual void    DestroyDynBuf( DynBufHandle ) {}

    virtual IContextBackend* CreateRenderContextBackend( Errors& ) { return {}; }

    static IRenderBackend3* sInstance;
  };
}
