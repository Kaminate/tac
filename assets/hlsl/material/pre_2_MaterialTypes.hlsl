#pragma pack_matrix( row_major )

// The MaterialParams struct represents the material parameters on the GPU.
// It reflects the CPU material paramter struct Tac::ConstBufData_Material in
// tac_material_presentation.cpp
//
// This is a collection of every possible data any material could need
// If your shader doesn't use a particular material parameter, it will have a default value
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

// The ShaderGraphParams struct represents the shader graph on the GPU.
// It reflects the CPU shader graph (Tac::ShaderGraph).
//
// It holds every possible parameter that a shader graph could use,
// so if your shader doesn't use a particular shader graph parameter, it will have a default value
struct ShaderGraphParams
{
  matrix mWorld;
  uint   mVertexBufferIndex;
  uint   mInputLayoutIndex;
};

struct PerFrameParams
{
  matrix mWorldToClip;
};


