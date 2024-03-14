#pragma once

namespace Tac
{
  struct Errors;

  struct DesktopAppErrorReport
  {
    static void Add( const char*, Errors* );
    static void Report();
  };

} // namespace Tac
