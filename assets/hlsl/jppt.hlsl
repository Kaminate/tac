
RWTexture2D<float4> sOutputTexture : register( u0 );

[numthreads(8,8,1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{
  uint width, height;
  sOutputTexture.GetDimensions(width, height);
  if (id.x < width && id.y < height)
  {
    float u = (float) id.x / width;
    float v = 1 - (float) id.y / height;
    float2 color_sRGB = float2(u, v);
    float2 color_linear = pow(color_sRGB, 2.2);
    // our texture is linear (rgba8unorm cannot be used with typed uav),
    // so when it is sampled, it will be... linear(?).
    // 
    // and our backbuffer is linear (rgba16f), so the pixel shader should return linear color
    // and this shader should return linear color which goes into the texture
    sOutputTexture[id.xy] = float4( color_linear, 0, 1);
  }
}
