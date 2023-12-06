#include "Common.hlsl"

#define TEST_RED 0
#define TEST_UVS 0

Texture2D image       : register( t0 );
sampler linearSampler : register( s0 );

struct VS_INPUT
{
  float3 Position   : POSITION;
  float2 GLTexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 DXTexCoord         : TAC_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.DXTexCoord.x = input.GLTexCoord.x;
  output.DXTexCoord.y = 1 - input.GLTexCoord.y;

  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  float4 sampled = image.Sample( linearSampler, input.DXTexCoord );

  // For reference, search https://en.wikipedia.org/wiki/Alpha_compositing
  // for "premultiplied" or "pre-multiplied"
  //     ^
  //     |
  // +-------+
  // | READ! |
  // +-------+

  // convert from sRGB, because our backbuffer is linear
  v4 linearColor = v4(pow( Color.rgb, 2.2 ), Color.a);
  v4 linearSampled = v4(pow( sampled.rgb, 2.2), sampled.a);

  //output.mColor.rgb = linearColor * sampled.rgb;
  output.mColor = linearColor * linearSampled;

  // Color is srgb, but our backbuffer is linear, so convert the
  // srgb color to linear to that they are both in linear space.
  //output.mColor.rgb =  pow( output.mColor.rgb, 2.2 ); // srgb to linear

#if TEST_RED
  output.mColor = float4( 1, 0, 0, 1 );
#endif

#if TEST_UVS
  output.mColor += float4( input.DXTexCoord.x, input.DXTexCoord.y, 0, 1 );
#endif

  return output;
}

