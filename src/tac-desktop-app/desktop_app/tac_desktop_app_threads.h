#pragma once

namespace Tac
{
  struct DesktopAppThreads
  {
    enum class ThreadType
    {
      Unknown,
      Sys,
      Sim
    };
    static void SetType( ThreadType );
    static bool IsType( ThreadType );
    static bool IsSysThread();
    static bool IsSimThread();
  };
} // namespace Tac
