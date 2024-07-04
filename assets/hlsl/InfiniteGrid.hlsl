struct ConstBufStruct
{
  row_major matrix mInvView;
  row_major matrix mInvProj;
  row_major matrix mViewProj;
  float4           mCamPos_ws;
};

struct VS_INPUT
{
  uint mVertexID  : SV_VertexID;
};

// This is the position of the corners of the near plane
struct VS_OUTPUT
{
  float4 mPos_cs : SV_POSITION;
  float4 mPos_ws : TAC_AUTO_SEMANTIC;
  float4 mPos_vs : TAC_AUTO_SEMANTIC;
};

typedef ConstantBuffer< ConstBufStruct > ConstBuf;

ConstBuf sConstants : TAC_AUTO_REGISTER( b );


// Note:
//   world   * xyz1_ls  = xyz1_ws
//   view    * xyz1_ws  = xyz1_vs
//   proj    * xyz1_vs  = xyzw_cs
//   xyzw_cs / w_cs     = xyz1_ns

// returns the clipspace vertex of a full screen 
float2 GetXY_ndc( uint iVtx )
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

  return ndcXYs[ iVtx ];
}

/*
float4 GetPos_cs( uint iVtx, float z_ndc )
{
  float2 xy_ndc = GetXY_ndc(iVtx);
  const float4 pos_cs = float4(xy_ndc, z_ndc, 1.0);
  return pos_cs;
}
*/

VS_OUTPUT VS( VS_INPUT input )
{
  const matrix invView = sConstants.mInvView;
  const matrix invProj = sConstants.mInvProj;
  const uint iVtx = input.mVertexID;

  const float2 nearplane_xy_ndc = GetXY_ndc( iVtx );
  const float4 nearplane_pos_vs_aux = mul( invProj, float4( nearplane_xy_ndc, 0, 1 ) );
  const float4 nearplane_pos_vs = nearplane_pos_vs_aux / nearplane_pos_vs_aux.w;
  const float4 nearplane_pos_ws = mul( invView, nearplane_pos_vs );

  VS_OUTPUT output;
  output.mPos_cs = float4( nearplane_xy_ndc, 0, 1 ); // not actually in clip space
  output.mPos_ws = nearplane_pos_ws;
  output.mPos_vs = nearplane_pos_vs;
  return output;
}

// ------------------------------------------------------------------------------------------------

struct PS_INPUT
{
  float4 mPos_ndc : SV_POSITION;
  float4 mPos_ws  : TAC_AUTO_SEMANTIC;
};

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
  float  mDepth : SV_Depth;
};

bool intersect( float4 rayPos, float4 rayDir, out float t )
{
  // plane eq: y = 0
  // ray eq: rO.y + t * rD.y = y
  //
  // therefore t = -rO.y / rD.y
  t = -rayPos.y / rayDir.y;
  if (t < 0)
    return false;

  return true;
}

PS_OUTPUT PS( PS_INPUT input )
{
  const matrix viewProj = sConstants.mViewProj;

  float4 rayPos_ws = sConstants.mCamPos_ws;
  float4 rayDir_ws = input.mPos_ws - sConstants.mCamPos_ws; // unnormalized

  float t;
  bool hit = intersect( rayPos_ws, rayDir_ws, t );
  if (!hit)
  {
    discard;
  }

  float4 hitPos_ws = rayPos_ws + rayDir_ws * t;
  float4 hitPos_cs = mul(viewProj, hitPos_ws);
  float4 hitPos_ns = hitPos_cs / hitPos_cs.w;

  PS_OUTPUT output;
  output.mColor = float4( frac( hitPos_ws.xz ), 0, 1 );
  output.mDepth = hitPos_ns.z;
  return output;
}

