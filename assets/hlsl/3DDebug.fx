
struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Color : COLOR;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float3 mColor : COLOR;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 viewSpacePosition = mul( View, float4( input.Position, 1 ) );
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
  output.mColor.xyz = input.mColor;
  output.mColor.w = 1;

  return output;
}
