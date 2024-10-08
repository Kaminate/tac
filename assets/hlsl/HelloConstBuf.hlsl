struct ClipSpacePosition3 { float3 mFloat3; };
struct ClipSpacePosition4 { float4 mFloat4; };
struct NDCSpacePosition3  { float3 mFloat3; };
struct NDCSpacePosition4  { float4 mFloat4; };
struct LinearColor3       { float3 mFloat3; };
struct LinearColor4       { float4 mFloat4; };

struct Vertex
{
  ClipSpacePosition3 mPosition;
  LinearColor3       mColor;
};

// I learned something about hlsl shader semantics today...
//
// The vertex shader output and pixel shader input can have different structs,
// but the data between them is ferried through the semantics.
// so in this example, TAC_AUTO_SEMANTIC won't work for the texture coordinates, because the
// variables have different names
//
// I could `typedef VSOutput PSInput;`, but the position is technically in NDC Space in the 
// vertex shader, and in Clip Space in the pixel shader, so what variable type would you use then?

struct VSOutput
{
  NDCSpacePosition4  mVSPosition  : SV_POSITION;
  LinearColor3       mVSColor     : VERTEX_TO_PIXEL_SHADER_COLOR;
};

struct PSInput
{
  ClipSpacePosition4 mPSPosition  : SV_POSITION;
  LinearColor3       mPSColor     : VERTEX_TO_PIXEL_SHADER_COLOR;
};

#pragma pack_matrix( row_major )

struct MyCBufType
{
  matrix mWorld;
  uint   mVertexBufferIndex;
};

ByteAddressBuffer            BufferTable[] : register( t0, space0 );
ConstantBuffer< MyCBufType > MyCBufInst    : register( b0 );

VSOutput VSMain( uint vertexID : SV_VertexID )
{
  const matrix world              = MyCBufInst.mWorld;
  const uint   mVertexBufferIndex = MyCBufInst.mVertexBufferIndex;

  const uint byteOffset = sizeof( Vertex ) * vertexID;
  const Vertex vertex = BufferTable[ mVertexBufferIndex ].Load < Vertex >( byteOffset );

  VSOutput result;
  result.mVSPosition = NDCSpacePosition4( mul( world, float4( vertex.mPosition, 1 ) ) );
  result.mVSColor = vertex.mColor;
  return result;
}

LinearColor4 PSMain( PSInput input ) : SV_TARGET
{
  return LinearColor4( float4( input.mPSColor, 1 ) );
}
