// Material.hlsl (begin)

#include "MaterialTypes.hlsl"
#include "InputLayout.hlsli"

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
  float4 mLinearColor : SV_Target0;
};

PS_OUTPUT PSMain( PS_INPUT input )
{
  const float4 materialColor = sMaterialParams.mColor;

  PS_OUTPUT result;
  result.mLinearColor = float4( 0, 0, 0, 1 );
  result.mLinearColor.xyz += materialColor.xyz;
  return result;
}


// Material.hlsl (end)

