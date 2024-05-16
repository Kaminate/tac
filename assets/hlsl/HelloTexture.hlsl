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
Texture2D         Textures[]    : register( t0, space1 );
Texture2D         Texture[2]       : register( t0, space2 );
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
  Texture2D textureHELLO = Texture[0];
  SamplerState samplerState = Samplers[ 0 ];
  const float4 sample_sRGB = texture.Sample( samplerState , input.mTexCoords.mFloat2 );
  const float4 sampleHELLO_sRGB = textureHELLO.Sample( samplerState , input.mTexCoords.mFloat2 );
  //const float4 sample = texture.SampleLevel( samplerState , input.mTexCoords.mFloat2,0 );
  //const float4 sampleHELLO = textureHELLO.SampleLevel( samplerState , input.mTexCoords.mFloat2, 0 );

  const float4 sample_linear = float4(pow(sample_sRGB.rgb, 2.2), sample_sRGB.a);
  const float4 sampleHELLO_linear = float4(pow(sampleHELLO_sRGB.rgb, 2.2), sampleHELLO_sRGB.a);

  // return LinearColor4(float4(input.mTexCoords.mValue, 0, 1) );
  return LinearColor4((sample_linear + sampleHELLO_linear) / 2.0);
}
