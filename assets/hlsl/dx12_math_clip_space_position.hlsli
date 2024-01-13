#pragma once


struct ClipSpacePosition3
{
  float3 mFloat3;
};

struct ClipSpacePosition4
{
  static ClipSpacePosition4 Ctor( ClipSpacePosition3, float w = 1.0 );
  void                      Set( ClipSpacePosition3 xyz, float w = 1.0 );

  float4 mFloat4;
};


