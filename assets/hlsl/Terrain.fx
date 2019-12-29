
Texture2D terrainTexture : register( t0 );
Texture2D noiseTexture : register( t1 );

sampler linearSampler : register( s0 );


struct VS_INPUT
{
  float3 Position : POSITION;
  float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  //float2 mWorldSpaceXZ : HI;// : POSITION;
  float2 mTexCoord : TEXCOORD;
  float4 mClipSpacePosition : SV_POSITION;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  //output.mWorldSpaceXZ = worldSpacePosition.xz;
  output.mTexCoord = input.TexCoord;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

static const bool showBaseUvs           = true;
static const bool showTerrain           = false;
static const bool showTerrainModulation = false;
static const bool showNoise             = false;
static const bool showNoiseModulation   = false;
static const bool showDomain            = true;
static const bool showNoiseScroll       = true;
static const bool showTiledResult       = true;

static float3 finalColor = float3( 0, 0, 0 );

void ShowModulation( float2 uvs )
{
  const float2 uvModded = fmod( uvs, 1.0 );
  const float exponent = 120.0;
  finalColor.xy += pow( uvModded, exponent );
  finalColor.xy += uvModded / 5.0;
}

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  const float3 sampledsRGB = terrainTexture.Sample(
    linearSampler,
    input.mTexCoord ).xyz;
  const float3 sampledLinear = pow( sampledsRGB, 2.2 );

  const float3 pixelColor = Color.xyz * sampledLinear.xyz;
  const float2 noiseuv = input.mTexCoord * 2.1 / 256.0;
  const float noiseSample = noiseTexture.Sample(
    linearSampler,
    noiseuv );



  if( showTiledResult )
    finalColor.xyz = pixelColor;

  if( showBaseUvs )
    ShowModulation( input.mTexCoord );

  output.mColor = float4( finalColor, 1.0 );

  return output;
}

