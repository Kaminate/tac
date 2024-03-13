#include "tac_render_backend.h" // self-inc

namespace Tac::Render
{
  static IBackend* sBackend;

  void IBackend::Set( IBackend* backend) { sBackend = backend; }

  IBackend* IBackend::Get() { return sBackend; }
} // namespace Tac::Render

