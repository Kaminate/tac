// 

enum CameraType
{
  CameraType_Perspective,
  CameraType_Orthographic,
};

struct ConstBufStruct
{
  row_major matrix mInvView;
  row_major matrix mInvProj;
  row_major matrix mViewProj;
  float4           mCamPos_ws;
  float4           mCamDir_ws;
  uint             mCamType;
};

struct VS_INPUT
{
  uint mVertexID  : SV_VertexID;
};

// This is the position of the corners of the near plane
struct VS_OUTPUT
{
  float4 mPos_cs : SV_POSITION;
  float4 mPos_ws : TAC_AUTO_SEMANTIC; // TODO: make this ALSO the rayPos for ORTHO!
  // float4 mPos_vs : TAC_AUTO_SEMANTIC;
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

  // https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/
  const float2 nearplane_xy_ndc = GetXY_ndc( iVtx );
  const float4 nearplane_pos_vs_aux = mul( invProj, float4( nearplane_xy_ndc, 0, 1 ) );
  const float4 nearplane_pos_vs = nearplane_pos_vs_aux / nearplane_pos_vs_aux.w;
  const float4 nearplane_pos_ws = mul( invView, nearplane_pos_vs );

  VS_OUTPUT output;
  output.mPos_cs = float4( nearplane_xy_ndc, 0, 1 ); // not actually in clip space
  output.mPos_ws = nearplane_pos_ws;
  // output.mPos_vs = nearplane_pos_vs;
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

// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
// http://iquilezles.org/articles/filterableprocedurals

float GetGridColor(float2 uv)
{
  const float lineWidth = 0.01f;
  const float2 uvDeriv = fwidth(uv);
  const float2 drawWidth = max(lineWidth, uvDeriv);
  float2 lineAA = uvDeriv * 1.5;
  float2 gridUV = 1 - abs(frac(uv) * 2.0 - 1.0);
  float2 grid = smoothstep(lineWidth + lineAA, lineWidth - lineAA, gridUV);
  grid *= saturate(lineWidth / drawWidth);
  return lerp(grid.x, 1.0, grid.y) ;
}

PS_OUTPUT PS( PS_INPUT input )
{
  const matrix viewProj = sConstants.mViewProj;

  float4 rayPos_ws = (float4)0;
  float4 rayDir_ws = (float4)0;

  if( sConstants.mCamType == CameraType_Perspective)
  {
    rayPos_ws = sConstants.mCamPos_ws;
    rayDir_ws = input.mPos_ws - sConstants.mCamPos_ws; // unnormalized
  }
  else if ( sConstants.mCamType == CameraType_Orthographic)
  {
    rayPos_ws = input.mPos_ws;
    rayDir_ws = sConstants.mCamDir_ws;
  }



  // Ignoring t < 0 is okay in perspectie (and necessarily in ortho???)
  float t = -rayPos_ws.y / rayDir_ws.y; // plane eq: y=0, ray eq: y=rO.y+t*rD.y

  float4 hitPos_ws = rayPos_ws + rayDir_ws * t;
  float4 hitPos_cs = mul(viewProj, hitPos_ws);
  float4 hitPos_ns = hitPos_cs / hitPos_cs.w;

  float grid = GetGridColor(hitPos_ws.xz);
  if( grid == 0)
    discard;

  PS_OUTPUT output;
  //output.mColor = float4(frac(hitPos_ws.xz), 0, 1);
  output.mColor = float4(grid, grid, grid, 1);
  output.mDepth = hitPos_ns.z;
  return output;
}

