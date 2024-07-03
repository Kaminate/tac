struct ConstBufStruct
{
  row_major matrix mInvView;
  row_major matrix mInvProj;
};

struct Vtx
{
  float3 Position;
  float3 Normal;
};

struct VS_INPUT
{
  uint mVertexID  : SV_VertexID;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition  : SV_POSITION;
};


typedef ConstantBuffer< ConstBufStruct > ConstBuf;

ConstBuf sConstants : TAC_AUTO_REGISTER( b );

VS_OUTPUT VS( VS_INPUT input )
{
  float4 fsPositions_cs[ 3 ] =
  {
    float4( -1, -1, 1, 1 ),
    float4( 1, -1, 1, 1 ),
    float4( -1, 1, 1, 1 ),
  };

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = fsPositions_cs[ input.mVertexID ];
  return output;
}

// ------------------------------------------------------------------------------------------------

struct PS_INPUT
{
  float4 mPos_ndc  : SV_POSITION;
};

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
  float  mDepth : SV_Depth;
};

PS_OUTPUT PS( PS_INPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor = float4( 1, 0, 0, 1 );
  output.mDepth = ;
  return output;
}

