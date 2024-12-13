#pragma once

namespace Tac
{
  struct DesktopAppThreads
  {
    enum class ThreadType
    {
      Unknown,
      Sys,
      Sim,
      App
    };
    static void SetType( ThreadType );
    static bool IsType( ThreadType );
    static bool IsSysThread();
    static bool IsSimThread();
    static bool IsAppThread();
  };
} // namespace Tac
