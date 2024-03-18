#pragma once

namespace Tac{ struct Errors; }
namespace Tac::Render
{
  struct IRenderBackend3
  {
    IRenderBackend3();
    virtual void Init( Errors& ) {};
    static IRenderBackend3* sInstance;
  };
}
