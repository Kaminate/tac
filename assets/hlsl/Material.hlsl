struct Material
{
  float3 mAlbedo;
  float3 mNormal;
};

TAC_DEFINE_BITFIELD_BEGIN;
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRMetallicRoughness, 1 );
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRSpecularGlossiness, 1 );
TAC_DEFINE_BITFIELD_END;

struct MaterialInput
{
  float4   mColor;
  float3   mEmissive;
  uint     mFlags;

  // texture indexes
  // vertex buffer output?
};

Material GetMaterial( MaterialInput );


struct InputLayout
{

  int mStride;
};


#pragma pack_matrix( row_major )

struct CBufType
{
  matrix mWorld;
  uint   mVtxBufIdx;
  uint   mInputLayoutIdx;
};

// -- BEGIN MTL SHD COMPILE -----------------------------------

struct VS_OUTPUT
{
  float4 mPos_NDC : SV_POSITION;
};

// -- END MTL SHD COMPILE -----------------------------------

ConstantBuffer< CBufType > sCBuf      : register( b0 )
ByteAddressBuffer          sVtxBufs[] : register( t0, space0 );

VS_OUTPUT VSMain( uint iVtx : SV_VertexID )
{
  const matrix world = sCBuf.mWorld;
  const uint vtxBufIdx = sCBuf.mVtxBufIdx;

  const InputLayout inputLayout = sVtxBufs[ sCBuf.mInputLayoutIdx ].Load< InputLayout >( 0 );

  const uint vtxByteOffset = sVtxBufs[ sCBuf.mVtxBufIdx ].Load<  * ;

// -- BEGIN MTL SHD COMPILE -----------------------------------

  VS_OUTPUT GetVtxOutput()
  
  
// -- END MTL SHD COMPILE -----------------------------------


}


// -------------------------

struct PS_INPUT
{
};

struct PS_OUTPUT
{
  float4 mLinearColor;
};

PS_OUTPUT PSMain()
{

  PS_OUTPUT result;
  result.mLinearColor = float4( 1, 0, 0, 1 );
  return result;
}


