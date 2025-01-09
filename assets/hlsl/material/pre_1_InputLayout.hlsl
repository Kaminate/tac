
// Tac::Render::Attribte
enum class Attribute
{
  Unknown,
  Position,
  Normal,
  Texcoord,
  Color,
  BoneIndex,
  BoneWeight,
  Coeffs,
  Count
};

// Tac::Render::GraphicsType (prepended with 'k' to avoid keyword collision)
enum class GraphicsType
{
  kUnknown,
  kSint,
  kUint,
  kSnorm,
  kUnorm,
  kReal
};

// must mirror Tac::Render::GPUInputLayout in tac_gpu_input_layout.h
struct InputLayout
{
  GraphicsType GetGraphicsType( Attribute a )
  {
    GraphicsType result = ( GraphicsType )mGraphicsTypes[ ( uint )a ];
    return result;
  }

  uint         GetElementCount( Attribute a )
  {
    uint result = mElementCounts[ ( uint )a ];
    return result;
  }

  uint         GetByteOffset( Attribute a )  
  {
    uint result = mByteOffsets[ ( uint )a ];
    return result;
  }

  uint mGraphicsTypes[ 16 ];
  uint mElementCounts[ 16 ];
  uint mByteOffsets[ 16 ];
  uint mStride;
};

