
#include "common.fx"
Texture2D terrainTexture : register( t0 );
// Texture2D noiseTexture : register( t1 );

sampler linearSampler : register( s0 );


struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
  float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  float3 mWorldSpacePosition  : HI;
  float3 mWorldSpaceNormal    : NORMAL;
  float2 mTexCoord            : TEXCOORD0;
  float4 mClipSpacePosition   : SV_POSITION;
  float4 mScreenSpacePosition : TEXCOORD1;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mWorldSpacePosition = worldSpacePosition.xyz;
  output.mWorldSpaceNormal = input.Normal;
  output.mTexCoord = input.TexCoord;
  output.mScreenSpacePosition = clipSpacePosition;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};



PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;

  // uhh this is actually NDC space
  const float2 screenSpacePosition
    = input.mScreenSpacePosition.xy
    / input.mScreenSpacePosition.w; // [-1,1]^2 
  float3 finalColor = float3( 0, 0, 0 );

  output.mColor = float4( finalColor, 1.0 );

  return output;
}

