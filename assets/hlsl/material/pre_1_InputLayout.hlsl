
// Tac::Render::Attribte
enum class Attribute
{
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

struct InputLayout
{
  GraphicsType GetGraphicsType( Attribute a ) { return (GraphicsType)mGraphicsTypes[ ( uint )a ]; }
  uint         GetElementCount( Attribute a ) { return mElementCounts[ ( uint )a ]; }
  uint         GetByteOffset( Attribute a )   { return mByteOffsets[ ( uint )a ]; }

  uint mGraphicsTypes[ 16 ];
  uint mElementCounts[ 16 ];
  uint mByteOffsets[ 16 ];
  uint mStride;
};

