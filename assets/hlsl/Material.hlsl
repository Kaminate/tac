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

