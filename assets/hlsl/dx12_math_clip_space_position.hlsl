#include "dx12_math_clip_space_position.hlsli"

ClipSpacePosition4 ClipSpacePosition4::Ctor( const ClipSpacePosition3 xyz, float w )
{
  ClipSpacePosition4 result;
  result.Set( xyz, w );
  return result;
}

void ClipSpacePosition4::Set( const ClipSpacePosition3 xyz, float w )
{
  mFloat4 = float4( xyz, w );
}

