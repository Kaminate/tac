// gltfpbr.hlsl (begin)

struct VS_OUTPUT
{
  float4 mPos_clip_ndc : SV_POSITION;
};

VS_OUTPUT VSMain( uint iVtx : SV_VertexID )
{
  const matrix world          = sShaderGraphParams.mWorld;
  const uint   vtxBufIdx      = sShaderGraphParams.mVertexBufferIndex;
  const uint   inputLayoutIdx = sShaderGraphParams.mInputLayoutIndex;

  if( vtxBufIdx == ( uint )-1 )
    return (VS_OUTPUT)0;

  if( inputLayoutIdx == ( uint )-1 )
    return ( VS_OUTPUT )0;

  const InputLayout inputLayout = sBuffers[ inputLayoutIdx ].Load< InputLayout >( 0 );
  if( inputLayout.GetElementCount( Attribute::Position ) < 3 )
    return ( VS_OUTPUT )0;

  if( inputLayout.GetGraphicsType( Attribute::Position ) != GraphicsType::kReal )
    return ( VS_OUTPUT )0;

  uint posOffset = inputLayout.GetByteOffset( Attribute::Position );

  float3 pos3_objectspace = sBuffers[ vtxBufIdx ].Load< float3 >( inputLayout.mStride * iVtx + posOffset );
  float4 pos4_objectspace = float4( pos3_objectspace, 1 );

  const matrix worldToClip = sPerFrameParams.mWorldToClip;
  const float4 pos_worldspace = mul( world, pos4_objectspace );
  const float4 pos_clipspace = mul( worldToClip, pos_worldspace );

  VS_OUTPUT result;
  result.mPos_clip_ndc = pos_clipspace;

  return result;
}

// gltfpbr.hlsl (end)

