// gltfpbr.hlsl (begin)

struct VS_OUTPUT
{
  float4 mPos_clip_ndc : SV_POSITION;
};

template<typename T>
struct Optional
{
  T mValue;
  bool mHasValue;
};

Optional<float4> GetPosition_objectspace( uint iVtx )
{
  const uint vtxBufIdx = sShaderGraphParams.mVertexBufferIndex;
  if( vtxBufIdx == ( uint )-1 ) {return (Optional<float4>)0;}

  const uint inputLayoutIdx = sShaderGraphParams.mInputLayoutIndex;
  if( inputLayoutIdx == ( uint )-1 ) { return (Optional<float4>)0; }

  const InputLayout inputLayout = sBuffers[ inputLayoutIdx ].Load< InputLayout >( 0 );
  if( inputLayout.GetElementCount( Attribute::Position ) < 3 ) { return (Optional<float4>)0; }
  if( inputLayout.GetGraphicsType( Attribute::Position ) != GraphicsType::kReal ) { return (Optional<float4>)0; }

  uint posOffset = inputLayout.GetByteOffset( Attribute::Position );

  float3 pos3_objectspace = sBuffers[ vtxBufIdx ].Load< float3 >( inputLayout.mStride * iVtx + posOffset );

  Optional<float4> result;
  result.mHasValue = true;
  result.mValue = float4(pos3_objectspace, 1 );
  return result;
}

VS_OUTPUT VSMain( uint iVtx : SV_VertexID )
{
  Optional<float4> pos4_objectspace = GetPosition_objectspace( iVtx );

  const float4 pos_worldspace = mul( sShaderGraphParams.mWorld, pos4_objectspace.mValue );
  const float4 pos_clipspace = mul( sPerFrameParams.mWorldToClip, pos_worldspace );

  VS_OUTPUT result;
  result.mPos_clip_ndc = pos_clipspace;
  return result;
}

typedef VS_OUTPUT PS_INPUT;

struct PS_OUTPUT
{
  float4 mLinearColor : SV_Target0;
};

PS_OUTPUT PSMain( PS_INPUT input )
{
  const float4 materialColor = sMaterialParams.mColor;

  PS_OUTPUT result;
  result.mLinearColor = float4( 0, 0, 0, 1 );
  result.mLinearColor.xyz += materialColor.xyz;
  return result;
}

// gltfpbr.hlsl (end)

