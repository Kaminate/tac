#include "tac_render_backend.h" // self-inc

namespace Tac::Render
{
  IRenderBackend3* IRenderBackend3::sInstance;
  IRenderBackend3::IRenderBackend3()
  {
    sInstance = this;
  }
}
