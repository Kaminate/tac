#include "Common.hlsl"

#define TEST_RED 0
#define TEST_UVS 0
#define TEST_SDF_SHOW_ONEEDGE 0 // this dont work

Texture2D    atlas         : TAC_AUTO_REGISTER;
SamplerState linearSampler : TAC_AUTO_REGISTER;

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
  float2 uv = input.DXTexCoord;

  //float4 sampled = atlas.Sample( linearSampler, input.DXTexCoord );
  float sampled = atlas.Sample( linearSampler, uv ).r;

  // For reference, search https://en.wikipedia.org/wiki/Alpha_compositing
  // for "premultiplied" or "pre-multiplied"
  //     ^
  //     |
  // +-------+
  // | READ! |
  // +-------+

  float4 color = Color;

  sdfPixelDistScale; // [0, 1]
  float idk = 30 / 255.0f;

  //float a = 
  // float duv = fwidth( uv ); // fwidth(uv) = abs( ddx( uv ) ) + abs( ddy( uv ) );
  // texel-per-pixel density. change in uv / change in screen pixels

  // float dtex = length( float2( duv, duv ) * float2( ddx;

  // smoothstep(min,max,x)
  //   if x is less than min returns 0;
  //   if x is greater than max returns 1;
  //   otherwise, a value between 0 and 1 if x is in the range [min, max].
  float ss = smoothstep( sdfOnEdge - idk, sdfOnEdge + idk, sampled * 255.0 );

  color *= ss; // rgb and a cuz premultiplied alpha (?)

  // color = sampled * 255.0 > oneedge ? float4( 1,1,1,1) : float4( 0,0,0,0);
  // color = float4( 1,1,1,1 ) * sampled;

#if TEST_RED
  color = float4( 1, 0, 0, 1 );
#endif

#if TEST_UVS
  color.xy += input.DXTexCoord * 1.0f;
#endif

#if TEST_SDF_SHOW_ONEEDGE
  float oe = smoothstep( 5, 0, abs( sampled * 255.0 - oneedge ) );
  if( oe >= .50 )
    color.xyz = float3( 1, 0, 0 ) * oe;
#endif

  // The quick brown fox jumps over the lazy dog

  float pxDist = -(sampled - sdfOnEdge) / sdfPixelDistScale;
  color = float4( 1, 1, 1, 1 ) * saturate( 0.5 - pxDist );

  PS_OUTPUT result;
  result.mColor = color;
  return result;

  // return output;
}

