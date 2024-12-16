struct Debug3DCBufType
{
  row_major matrix mView;
  row_major matrix mProj;
};

struct VS_INPUT
{
  float3 Position : POSITION;
  float4 Color    : COLOR;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float4 mColor             : mColor;

};

typedef ConstantBuffer< Debug3DCBufType > Debug3DConstBuf;

Debug3DConstBuf constBuf : register(b0);

VS_OUTPUT VS( VS_INPUT input )
{
  matrix View = constBuf.mView;
  matrix Proj = constBuf.mProj;

  float4 viewSpacePosition = mul( View, float4( input.Position, 1 ) );
  float4 clipSpacePosition = mul( Proj, viewSpacePosition );

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
  output.mColor = input.mColor;
  return output;
}


