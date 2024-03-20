#pragma once

#include "tac_render_api.h"

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct IRenderBackend3
  {
    IRenderBackend3();
    virtual void Init( Errors& ) {};

    virtual FBHandle CreateFB( const void*, v2i ) { return {}; }
    virtual void     ResizeFB( FBHandle, v2i ) {}
    virtual void     DestroyFB( FBHandle ) {}

    static IRenderBackend3* sInstance;
  };
}
