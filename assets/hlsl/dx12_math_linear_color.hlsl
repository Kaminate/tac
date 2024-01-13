#include "dx12_math_linear_color.hlsli"

void LinearColor4::Set(const LinearColor3 xyz, float w )
{
  mFloat4 = float4( xyz.mFloat3, w );
}

LinearColor4 LinearColor4::Ctor(const LinearColor3 xyz, float w )
{
  LinearColor4 result;
  result.Set( xyz, w );
  return result;
}


