struct ConstBufStruct
{
  row_major matrix mInvView;
  row_major matrix mInvProj;
  float            mN;
  float            mF;
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
  float4 mPos_cs  : SV_POSITION;
  float4 mNearPlanePos_ws : TAC_AUTO_SEMANTIC;
  float4 mFarPlanePos_ws : TAC_AUTO_SEMANTIC;
};


typedef ConstantBuffer< ConstBufStruct > ConstBuf;

ConstBuf sConstants : TAC_AUTO_REGISTER( b );


// Note:
//   world   * xyz1_ls  = xyz1_ws
//   view    * xyz1_ws  = xyz1_vs
//   proj    * xyz1_vs  = xyzw_cs
//   xyzw_cs / w_cs     = xyz1_ns

// returns the clipspace vertex of a full screen 
float4 GetPos_cs( uint iVtx, float z_ndc )
{
  const float2 tl = float2( -1, 1 );
  const float2 tr = float2( 1, 1 );
  const float2 bl = float2( -1, -1 );
  const float2 br = float2(  1, -1 );

  const float2 ndcXYs[ 6 ] =
  {
    bl, br, tl,
    tr, tl, br,
  };

  const float4 pos_ns = float4( ndcXYs[ input.mVertexID ], z_ndc, 1 );


  const float4 pos_cs = ;
  return pos_cs;
}

VS_OUTPUT VS( VS_INPUT input )
{
  const matrix invView = sConstants.mInvView;
  const matrix invProj = sConstants.mInvProj;

  const float4 fpos_cs = GetPos_cs( input.mVertexID, 1.0f );
  const float4 fpos_vs = mul( invProj, pos_cs );
  const float4 fpos_ws = mul( invView, pos_vs );

  const float4 npos_cs = GetPos_cs( input.mVertexID, 0.0f ); // note: this is -1 in opengl
  const float4 npos_vs = mul( invProj, pos_cs );
  const float4 npos_ws = mul( invView, pos_vs );

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mPos_cs = fpos_cs;
  output.mNPos_ws = npos_ ws;
  output.mFPos_ws = fpos_ws;
  return output;
}

// ------------------------------------------------------------------------------------------------

struct PS_INPUT
{
  float4 mPos_ndc : SV_POSITION;
  float4 mNPos_ws : TAC_AUTO_SEMANTIC;
  float4 mFPos_ws : TAC_AUTO_SEMANTIC;
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

