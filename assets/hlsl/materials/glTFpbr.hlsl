// gltfpbr.hlsl (begin)

struct VS_OUTPUT
{
  float4 mPos_clip_ndc : SV_POSITION;
};

VS_OUTPUT VSMain( uint iVtx : SV_VertexID )
{
  const matrix world = sShaderGraphParams.mWorld;
  const uint vtxBufIdx = sShaderGraphParams.mVtxBufIdx;
  const uint inputLayoutIdx = sShaderGraphParams.mInputLayoutIdx;

  const InputLayout inputLayout = sBuffers[ inputLayoutIdx ].Load< InputLayout >( 0 );

  const matrix worldToClip = sPerFrameParams.mWorldToClip;
  const float4 pos_worldspace = float4( 0, 0, 0, 1 );
  const float4 pos_clipspace = mul( worldToClip, pos_worldspace );

  VS_OUTPUT result;
  result.mPos_clip_ndc = pos_clipspace;

  if( true ) // temp
  {
    result.mPos_clip_ndc.x += (float)inputLayout.mStride;
  }
  return result;
}

// gltfpbr.hlsl (end)

