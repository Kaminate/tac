#define TEST_RED 0
#define TEST_UVS 0

Texture2D atlas : register( t0 );
sampler linearSampler : register( s0 );

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 DXTexCoord : TEXCOORD0;
};

VS_OUTPUT VS( ShaderInputText input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.DXTexCoord.x = input.GLTexCoord.x;
  output.DXTexCoord.y = 1 - input.GLTexCoord.y;

  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  float4 sampled = atlas.Sample( linearSampler, input.DXTexCoord );
  PS_OUTPUT output = ( PS_OUTPUT )0;

  // For reference, search https://en.wikipedia.org/wiki/Alpha_compositing
  // for "premultiplied" or "pre-multiplied"
  //     ^
  //     |
  // +-------+
  // | READ! |
  // +-------+

  // The .x here differentiates the text and sprite shaders
  //                              |
  //                              v
  output.mColor = Color * sampled.x;


#if TEST_RED
  output.mColor = float4( 1, 0, 0, 1 );
#endif

#if TEST_UVS
  output.mColor += float4( input.DXTexCoord.x, input.DXTexCoord.y, 0, 1 );
#endif

  return output;
}

