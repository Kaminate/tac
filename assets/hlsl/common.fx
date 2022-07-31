#define vec2 float2
#define vec3 float3
#define vec4 float4
#define v2   float2
#define v3   float3
#define v4   float4

cbuffer CBufferPerFrame : TAC_AUTO_REGISTER
{
  row_major matrix View;
  row_major matrix Projection;
  float            far;
  float            near;
  float2           gbufferSize;
  float            secModTau;
}

cbuffer CBufferPerObject : TAC_AUTO_REGISTER
{
  row_major matrix World;

  // Is this premultiplied alpha?
  float4           Color;
}

