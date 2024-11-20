
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

struct FormatElement
{
};

struct InputLayoutElement
{
  uint mType           : 8;
  uint mByteOffset     : 8;
  uint mComponentCount : 8;
  uint                 : 8;
};

struct InputLayout
{
  InputLayoutElement mElements[ ( uint )Attribute::Count ];
  uint mStride;
};

