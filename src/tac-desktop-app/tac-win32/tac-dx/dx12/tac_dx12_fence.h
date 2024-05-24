#pragma once

#include "tac-std-lib/tac_ints.h"

namespace Tac::Render
{
    // Signal value added to a ID3D12CommandQueue. When the GPU executes this signal, it sets
    // the fence to equal that value.
    struct FenceSignal
    {
      bool operator <( const FenceSignal& rhs ) const { return mValue < rhs.mValue; }
      FenceSignal( u64 value = 0 ) : mValue( value )  {};
      u64 GetValue() const                            { return mValue; }

    private:
      u64 mValue {};
    };
} // namespace Tac::Render

