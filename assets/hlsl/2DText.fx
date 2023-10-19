#include "Common.fx"

#define TEST_RED 0
#define TEST_UVS 0

Texture2D atlas       : register( t0 );
sampler linearSampler : register( s0 );

struct VS_INPUT
{
  float3 Position   : POSITION;
  float2 GLTexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 DXTexCoord         : SV_AUTO_SEMANTIC;
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
  //float4 sampled = atlas.Sample( linearSampler, input.DXTexCoord );
  float sampled = atlas.Sample( linearSampler, input.DXTexCoord );

  // For reference, search https://en.wikipedia.org/wiki/Alpha_compositing
  // for "premultiplied" or "pre-multiplied"
  //     ^
  //     |
  // +-------+
  // | READ! |
  // +-------+

  // float4 color = Color;
  float4 color = Color;

  float oneedge = 128.0;
  float idk = 30;

  // smoothstep(min,max,x)
  //   if x is less than min returns 0;
  //   if x is greater than max returns 1;
  //   otherwise, a value between 0 and 1 if x is in the range [min, max].
  float ss = smoothstep( oneedge - idk, oneedge + idk, sampled * 255.0 );

  color *= ss;

#if TEST_RED
  color = float4( 1, 0, 0, 1 );
#endif

#if TEST_UVS
  color.xy += input.DXTexCoord * 1.0f;
#endif

  PS_OUTPUT result;
  result.mColor = color;
  return result;

  // return output;
}

