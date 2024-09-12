#pragma pack_matrix( row_major )

// Material (Tac::Material) parameters
struct MaterialParams
{
  float4 mColor;
  float4 mEmissive;
  uint   mMaterialFlags;
  uint   mDiffuseTextureIdx;
  uint   mSpecularTextureIdx;
};

// Shader Graph (Tac::ShaderGraph) parameters
struct ShaderGraphParams
{
  matrix mWorld;
  uint   mVtxBufIdx;
  uint   mInputLayoutIdx;
};

struct PerFrameParams
{
  matrix mWorldToClip;
};

// MaterialParams::mMaterialFlags
TAC_DEFINE_BITFIELD_BEGIN;
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRMetallicRoughness, 1 );
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRSpecularGlossiness, 1 );
TAC_DEFINE_BITFIELD_END;


