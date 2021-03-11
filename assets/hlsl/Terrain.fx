
Texture2D terrainTexture : register( t0 );
Texture2D noiseTexture : register( t1 );

sampler linearSampler : register( s0 );


struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
  float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  float3 mWorldSpacePosition  : HI;
  float3 mWorldSpaceNormal    : NORMAL;
  float2 mTexCoord            : TEXCOORD0;
  float4 mClipSpacePosition   : SV_POSITION;
  float4 mScreenSpacePosition : TEXCOORD1;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mWorldSpacePosition = worldSpacePosition.xyz;
  output.mWorldSpaceNormal = input.Normal;
  output.mTexCoord = input.TexCoord;
  output.mScreenSpacePosition = clipSpacePosition;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

static const bool showBaseUvs           = false;
static const bool showTerrain           = false;
static const bool showTerrainModulation = false;
static const bool showNoise             = false;
static const bool showNoiseModulation   = false;
static const bool showNoiseDomain       = false;
static const bool showNoiseScroll       = false;
static const bool showTiledResult       = true;
static const bool showTiledResultOrig   = false;

static float3 finalColor = float3( 0, 0, 0 );

// turns a number into a random direction
float2 Hash( float i )
{
    return sin( float2( 3.0, 7.0 ) * i );
}

void ShowModulation( float2 uvs )
{
  const float2 uvModded = fmod( uvs, 1.0 );
  const float exponent = 120.0;
  finalColor.xy += pow( uvModded, exponent );
  finalColor.xy += uvModded / 5.0;
}

float sum( float3 v )
{
  return v.x + v.y + v.z;
}

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  const float magicNoiseScalar = 1000.0;
  const float2 noiseuv = input.mWorldSpacePosition.xz / magicNoiseScalar;
  const float noiseSample = noiseTexture.Sample( linearSampler,
                                                 noiseuv ).x;
  const float noiseIndex = noiseSample * 8.0;
  const float noiseIndexWhole = floor( noiseIndex );
  const float noiseIndexFract = frac( noiseIndex );

  if( showNoise )
    finalColor.xyz += noiseSample;

  if( showNoiseModulation )
    ShowModulation( noiseuv );

  if( showBaseUvs )
    ShowModulation( input.mTexCoord );

  float3 domainColor = float3( 0, 0, 0 );
  if( showNoiseDomain )
  {
    float3 domainColors[ 8 ];
    domainColors[ 0 ] = float3( 0, 0, 0 );
    domainColors[ 1 ] = float3( 0, 0, 1 );
    domainColors[ 2 ] = float3( 0, 1, 0 );
    domainColors[ 3 ] = float3( 0, 1, 1 );
    domainColors[ 4 ] = float3( 1, 0, 0 );
    domainColors[ 5 ] = float3( 1, 0, 1 );
    domainColors[ 6 ] = float3( 1, 1, 0 );
    domainColors[ 7 ] = float3( 1, 1, 1 );
    float3 domainColor = domainColors[ int( noiseIndexWhole ) ];
    for(float border = 1.0; border <= 8.0; border += 1.0 )
    {
        if( noiseIndex < border )
        {
            finalColor += domainColor * 0.1;
            break;
        }
    }

    finalColor += pow( 1.0 - noiseIndexFract, 10.0 ) * domainColor;
  }

  float2 offa = Hash( noiseIndexWhole + 0.0 );
  float2 offb = Hash( noiseIndexWhole + 1.0 );
  // the variable 'v' controls how much we add the offset direction to our sample
  float v = 69.0;
  if( showTiledResultOrig )
    v = 0.0;
  float2 dvudx = ddx( input.mTexCoord );
  float2 dvudy = ddy( input.mTexCoord );
  float3 cola = terrainTexture.SampleGrad( linearSampler, v * offa + input.mTexCoord, dvudx, dvudy ).xyz;
  float3 colb = terrainTexture.SampleGrad( linearSampler, v * offb + input.mTexCoord, dvudx, dvudy ).xyz;

  //float mip = 0.0;
  //float3 cola = terrainTexture.SampleLevel( linearSampler, v * offa + input.mTexCoord, mip ).xyz;
  //float3 colb = terrainTexture.SampleLevel( linearSampler, v * offb + input.mTexCoord, mip ).xyz;

  //float3 cola = terrainTexture.Sample( linearSampler, v * offa + input.mTexCoord).xyz;
  //float3 colb = terrainTexture.Sample( linearSampler, v * offb + input.mTexCoord).xyz;

  if( showTiledResult )
  {
    float t = smoothstep( 0.2, 0.8, noiseIndexFract - 0.1 * sum( cola - colb ) );
    finalColor += pow( lerp( cola, colb, t ), 2.2 );
  }

/*
  if( showTiledResultOrig )
  {
    const float3 sampledsRGB = terrainTexture.Sample(
      linearSampler,
      input.mTexCoord ).xyz;
    const float3 sampledLinear = pow( sampledsRGB, 2.2 );
    const float3 pixelColor = Color.xyz * sampledLinear.xyz;
    finalColor.xyz += pixelColor;
  }
  */

  // uhh this is actually NDC space
  const float2 screenSpacePosition = input.mScreenSpacePosition.xy / input.mScreenSpacePosition.w; // [-1,1]^2

  // if( screenSpacePosition.x > sin( secModTau * 3.) )
  finalColor *= dot( -input.mWorldSpaceNormal, float3( 0, 1, 0) ); // Simple lighting


  output.mColor = float4( finalColor, 1.0 );

  return output;
}

