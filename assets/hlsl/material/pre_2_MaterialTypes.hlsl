#pragma pack_matrix( row_major )

// Material (Tac::Material) parameters
// This is a collection of every possible data any material could need
struct MaterialParams
{
  float4 mColor;
  float4 mEmissive;
  uint   mMaterialFlags;
  uint   mDiffuseTextureIdx;
  uint   mSpecularTextureIdx;
};

// MaterialParams::mMaterialFlags
TAC_DEFINE_BITFIELD_BEGIN;
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRMetallicRoughness, 1 );
TAC_DEFINE_BITFIELD_ELEMENT( IsGLTFPBRSpecularGlossiness, 1 );
TAC_DEFINE_BITFIELD_END;


// Shader Graph (Tac::ShaderGraph) parameters
struct ShaderGraphParams
{
  matrix mWorld;
};

struct PerFrameParams
{
  matrix mWorldToClip;
};


