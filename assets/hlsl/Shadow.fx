#include "common.fx"
// TextureCube cubemap   : register( t0 );
// sampler linearSampler : register( s0 );

struct VS_INPUT
{
  float3 Position : POSITION;
};

struct VS_OUTPUT
{
  // float4 mClipSpacePosition : SV_POSITION;
  // float3 mViewSpacePosition : SV_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  // float4 viewSpacePosition = mul( View, float4( input.Position, 1 ) );
  // float4 clipSpacePosition = mul( Projection, float4( input.Position, 1 ) );

  // Shader optimization trick which causes the ndc position to be
  // on the far plane, thus allowing the skybox to be rendered after
  // all other objects, and can skip out on the pixel shader where it
  // fails the depth test.
  // clipSpacePosition.z = clipSpacePosition.w;

  VS_OUTPUT output = ( VS_OUTPUT )0;
  // output.mClipSpacePosition = clipSpacePosition;
  // output.mViewSpacePosition = viewSpacePosition.xyz;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  // float3 sampled_sRGB = cubemap.Sample( linearSampler, input.mViewSpacePosition ).xyz;
  // float3 sampled_linear = pow( sampled_sRGB, 2.2 );

  PS_OUTPUT output = ( PS_OUTPUT )0;
  // output.mColor = float4( sampled_linear, 1 );

  return output;
}

