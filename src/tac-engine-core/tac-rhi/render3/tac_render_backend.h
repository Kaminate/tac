#pragma once

#include "tac_render_api.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct IRenderBackend3
  {
    IRenderBackend3();
    virtual void Init( Errors& ) {};

    using PostFBDestroy = void( * )( FBHandle );
    using PostDBDestroy = void( * )( DynBufHandle );

    virtual void CreateFB( FBHandle, const void*, v2i, Errors& ) {}
    virtual void ResizeFB( FBHandle, v2i ) {}
    virtual void DestroyFB( FBHandle, PostFBDestroy ) {}

    virtual void CreateDynBuf( DynBufHandle, int, StackFrame, Errors& ) {}
    virtual void UpdateDynBuf( RenderApi::UpdateDynBufParams ) {}
    virtual void DestroyDynBuf( DynBufHandle, PostDBDestroy ) {}

    static IRenderBackend3* sInstance;
  };
}
