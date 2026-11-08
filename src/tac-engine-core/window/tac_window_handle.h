// TODO: The prupose of this file is to define the WindowHandle struct, as used in 
// the sim and sys window apis (tac_sim_window_api.h, tac_sys_window_api.h)

#pragma once

#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct NativeWindowHandle
  {
    bool operator == ( const NativeWindowHandle& ) const = default;
    const void* mPointer;
  };

  struct WindowHandle
  {
    enum Idx : u16 {};
    enum Gen : u16 {}; // Generation
    WindowHandle() = default;
    WindowHandle( Idx idx, Gen gen ) : mIdx( idx ), mGen( gen ) {}
    bool IsValid() const;
    auto GetIdx() const { return mIdx; }
    auto GetGen() const { return mGen; }
    bool operator ==( const WindowHandle& ) const = default;
    bool operator !=( const WindowHandle& ) const = default;
  private:
    Idx mIdx{};
    Gen mGen{}; // starts at 1 for valid windows
  };
} // namespace Tac


