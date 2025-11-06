#pragma once

namespace Tac
{
  struct Errors;

  struct DesktopAppErrorReport
  {
    static void Add( const char*, Errors* );
    static void Report();
    static void Report(Errors*);
  };

} // namespace Tac
