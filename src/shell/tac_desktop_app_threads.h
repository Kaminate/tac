#pragma once

namespace Tac
{
  struct DesktopAppThreads
  {
    enum class ThreadType
    {
      Unknown,
      Main,
      Logic
    };
    static void SetType( ThreadType );
    static bool IsType( ThreadType );
    static bool IsMainThread();
    static bool IsLogicThread();
  };
} // namespace Tac
