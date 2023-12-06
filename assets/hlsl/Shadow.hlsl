#include "Common.hlsl"

struct VS_INPUT
{
  float3 Position           : POSITION;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float4 debug_view_pos     : TAC_AUTO_SEMANTIC;
  float3 debug_ndc_pos      : TAC_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition );

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.debug_view_pos = viewSpacePosition;
  output.debug_ndc_pos = clipSpacePosition.xyz / clipSpacePosition.w;
  return output;
}

void PS( VS_OUTPUT input )
{
}

