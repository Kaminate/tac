#pragma pack_matrix( row_major )

struct VS_INPUT
{
  float3 Position : POSITION;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
};

struct PerObj
{
  matrix mWorld;
  float4 mColor;
};

struct PerFrame
{
  matrix mView;
  matrix mProj;
};


ConstantBuffer< PerFrame > perFrame : register( b0 );
ConstantBuffer< PerObj >   perObj   : register( b1 );

VS_OUTPUT VS( VS_INPUT input )
{
  matrix world = perObj.mWorld;
  matrix view = perFrame.mView;
  matrix proj = perFrame.mProj;

  float4 worldSpacePosition = mul( world, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( view, worldSpacePosition );
  float4 clipSpacePosition = mul( proj, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  float4 color = perObj.mColor;

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor.xyz = pow( color.xyz, 1.0 / 2.2 );
  output.mColor.w = color.w;

  return output;
}

