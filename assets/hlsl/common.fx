// worth?
#define vec3 float3
#define v3   float3

cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  row_major matrix Projection;
  float            far;
  float            near;
  float2           gbufferSize;

  // float            truncSecs;
  // double           elapsedAppSeconds;
  float            secModTau;
}

cbuffer CBufferPerObject : register( b1 )
{
  row_major matrix World;
  // Is this premultiplied alpha?
  float4           Color;
}

