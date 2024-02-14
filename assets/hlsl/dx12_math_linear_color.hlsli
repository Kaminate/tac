#pragma once

struct LinearColor3
{
  float3 mFloat3;
};

struct LinearColor4
{
  static LinearColor4 Ctor( LinearColor3 xyz, float w = 1.0 );

  float4 mFloat4;
};

