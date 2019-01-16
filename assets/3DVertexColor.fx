
struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float3 mColor : Color;
};

VS_OUTPUT VS( ShaderInputPosColor input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mColor = input.Color;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor = Color;
  output.mColor.xyz *= pow( input.mColor, 1.0 / 2.2 );

  return output;
}

