cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  row_major matrix Projection;
  float far;
  float near;
  float2 gbufferSize;
}

cbuffer CBufferPerObject : register( b1 )
{
  row_major matrix World;
  // Is this premultiplied alpha?
  float4 Color;
}

