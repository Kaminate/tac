#pragma once

#include "tac_render_api.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct IRenderBackend3
  {
    IRenderBackend3();
    virtual void Init( Errors& ) {};

    virtual void   CreateFB( FBHandle, FrameBufferParams, Errors& ) {}
    virtual void   ResizeFB( FBHandle, v2i ) {}
    virtual TexFmt GetFBFmt( FBHandle ) { return TexFmt::kUnknown; }
    virtual void   DestroyFB( FBHandle ) {}

    virtual void CreateDynBuf( DynBufHandle, int, StackFrame, Errors& ) {}
    virtual void UpdateDynBuf( RenderApi::UpdateDynBufParams ) {}
    virtual void DestroyDynBuf( DynBufHandle ) {}

    virtual void CreateShaderProgram( ProgramHandle, ShaderProgramParams, Errors& ) {}
    virtual void DestroyShaderProgram( ProgramHandle ) {}

    virtual void CreateRenderPipeline( PipelineHandle, PipelineParams, Errors& ) {}
    virtual void DestroyRenderPipeline( PipelineHandle ) {}

    static IRenderBackend3* sInstance;
  };
}
