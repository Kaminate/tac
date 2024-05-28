struct ClipSpacePosition3 { float3 mFloat3; };
struct ClipSpacePosition4 { float4 mFloat4; };

struct TextureCoordinate2 { float2 mFloat2; };

struct LinearColor3 { float3 mFloat3; };
struct LinearColor4 { float4 mFloat4; };

struct Vertex
{
  ClipSpacePosition3 mPosition;
  TextureCoordinate2 mTexCoords;
};

struct VSOutput
{
  ClipSpacePosition4 mPosition  : SV_POSITION;
  TextureCoordinate2 mTexCoords : TAC_AUTO_SEMANTIC;
};

typedef VSOutput PSInput;

ByteAddressBuffer BufferTable[] : register( t0, space0 );
#if 0 // bindless!
Texture2D         Textures[]    : register( t0, space1 );
#else // can see texture in PIX debugger
Texture2D         Textures[1]    : register( t0, space1 );
#endif
SamplerState      Samplers[]    : register( s0, space0 );

VSOutput VSMain( uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof( Vertex ) * iVtx;
  const Vertex input = BufferTable[ 0 ].Load < Vertex >( byteOffset );

  VSOutput result;
  result.mPosition = ClipSpacePosition4( float4( input.mPosition.mFloat3, 1.0 ) );
  result.mTexCoords = input.mTexCoords;
  return result;
}

LinearColor4 PSMain( PSInput input ) : SV_TARGET
{
  Texture2D texture = Textures[ 0 ];
  SamplerState samplerState = Samplers[ 0 ];

  // Because the texture format is SRGB (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB),
  // the sample operation converts it to linear.
  const float4 textureSample = texture.Sample( samplerState , input.mTexCoords.mFloat2 );
  return LinearColor4( textureSample );
}
