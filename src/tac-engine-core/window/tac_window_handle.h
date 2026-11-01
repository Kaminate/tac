// TODO: The prupose of this file is to define the WindowHandle struct, as used in 
// the sim and sys window apis (tac_sim_window_api.h, tac_sys_window_api.h)

#pragma once

#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct WindowHandle
  {
    //WindowHandle( int index = -1 ) : mIndex( index ) {}

    //bool IsValid() const  { return mIndex != -1; }
    //auto GetIndex() const { return mIndex; }
    //auto GetGeneration() const { return mGeneration; }

    // Comparison operators so client can compare search among their handles
    // without having to call GetIndex()
    //bool operator ==( const WindowHandle& ) const = default;
    //bool operator !=( const WindowHandle& ) const = default;

    u16 mIndex{};
    u16 mGeneration{};
  };
} // namespace Tac


