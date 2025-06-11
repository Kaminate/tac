// bakedRadiosity.hlsl (begin)

struct VS_OUTPUT
{
  float4 mPos_clip_ndc : SV_POSITION;
  float3 mVertexColor : TAC_AUTO_SEMANTIC;
};

float4 GetPosClipspace( uint iVtx )
{
  const matrix world          = sShaderGraphParams.mWorld;
  const uint   vtxBufIdx      = sShaderGraphParams.mVertexBufferIndex;
  const uint   inputLayoutIdx = sShaderGraphParams.mInputLayoutIndex;

  if( vtxBufIdx == ( uint )-1 )
    return float4(0,0,0,0);

  if( inputLayoutIdx == ( uint )-1 )
    return float4(0,0,0,0);

  const InputLayout inputLayout = sBuffers[ inputLayoutIdx ].Load< InputLayout >( 0 );
  if( inputLayout.GetElementCount( Attribute::Position ) < 3 )
    return float4(0,0,0,0);

  if( inputLayout.GetGraphicsType( Attribute::Position ) != GraphicsType::kReal )
    return float4(0,0,0,0);

  uint posOffset = inputLayout.GetByteOffset( Attribute::Position );

  float3 pos3_objectspace = sBuffers[ vtxBufIdx ].Load< float3 >( inputLayout.mStride * iVtx + posOffset );
  float4 pos4_objectspace = float4( pos3_objectspace, 1 );

  const matrix worldToClip = sPerFrameParams.mWorldToClip;
  const float4 pos_worldspace = mul( world, pos4_objectspace );
  const float4 pos_clipspace = mul( worldToClip, pos_worldspace );
  return pos_clipspace;
}

float3 GetVertexColor( uint iVtx )
{
  const uint   vtxBufIdx      = sShaderGraphParams.mVertexBufferIndex;
  const uint   inputLayoutIdx = sShaderGraphParams.mInputLayoutIndex;

  if( vtxBufIdx == ( uint )-1 )
    return float3(0,0,0);

  if( inputLayoutIdx == ( uint )-1 )
    return float3(0,0,0);

  const InputLayout inputLayout = sBuffers[ inputLayoutIdx ].Load< InputLayout >( 0 );
  if( inputLayout.GetElementCount( Attribute::Color ) < 3 )
    return float3(0,0,0);

  if( inputLayout.GetGraphicsType( Attribute::Color ) != GraphicsType::kReal )
    return float3(0,0,0);

  uint offset = inputLayout.GetByteOffset( Attribute::Color );

  float3 vertexColor = sBuffers[ vtxBufIdx ].Load< float3 >( inputLayout.mStride * iVtx + offset );
  return vertexColor;
}

VS_OUTPUT VSMain( uint iVtx : SV_VertexID )
{
  const float4 pos_clipspace = GetPosClipspace( iVtx );
  const float3 vtxColor = GetVertexColor( iVtx );

  VS_OUTPUT result;
  result.mPos_clip_ndc = pos_clipspace;
  result.mVertexColor = vtxColor;

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
  result.mLinearColor.xyz += materialColor.xyz * 0.0001;
  result.mLinearColor.xyz += input.mVertexColor;
  return result;
}

// bakedRadiosity.hlsl (end)

