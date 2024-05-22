#define TEST_RED 0
#define TEST_UVS 0

#pragma pack_matrix( row_major )

struct VS_INPUT
{
  float3 Position   : POSITION;
  float2 GLTexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 mDXTexCoord        : TAC_AUTO_SEMANTIC;
};

struct PerObjectType
{
  float4 mColor;
  uint   mType; // 0: image, 1: text
};

struct PerFrameType
{
  row_major matrix mOrthoProj;
  float            mSDFOnEdge; // [0, 1] (128/255.0) = 0.5
  float            mSDFPixelDistScale; // [0, 1] 128/5.0/255.0 = .1
};

Texture2D                       image         : register( t0 );
SamplerState                    linearSampler : register( s0 );
ConstantBuffer< PerFrameType >  perFrame      : register( b0 );
ConstantBuffer< PerObjectType > perObject     : register( b1 );

VS_OUTPUT VS( VS_INPUT input )
{
  float4 clipSpacePosition = mul( perFrame.mOrthoProj, float4( input.Position, 1 ));

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mDXTexCoord.x = input.GLTexCoord.x;
  output.mDXTexCoord.y = 1 - input.GLTexCoord.y;

  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;

  if( perObject.mType == 0 )
  {
    const float4 sampled = image.Sample( linearSampler, input.mDXTexCoord );
    // see: premultiplied alpha https://en.wikipedia.org/wiki/Alpha_compositing

    // convert from sRGB, because our backbuffer is linear
    const float4 linearColor = float4( pow( perObject.mColor.rgb, 2.2 ), perObject.mColor.a );
    const float4 linearSampled = float4( pow( sampled.rgb, 2.2), sampled.a );

    output.mColor = linearColor * linearSampled;
  }
  else if( perObject.mType == 1 )
  {
    const float sampled = image.Sample( linearSampler, input.mDXTexCoord ).r;

    // screenspace pixel distance from the current pixel to the edge of the isosurface
    const float pxDist = ( perFrame.mSDFOnEdge - sampled ) / perFrame.mSDFPixelDistScale;
    const float a = saturate( 0.5 - pxDist );

    // premultiplied alpha
    output.mColor = float4(perObject.mColor.rgb * a, perObject.mColor.a);
  }
  else
  {
    output.mColor = float4( 1, 1, 0, 1 );
  }

#if TEST_RED
  output.mColor = float4( 1, 0, 0, 1 );
#endif

#if TEST_UVS
  output.mColor += float4( input.mDXTexCoord.x, input.mDXTexCoord.y, 0, 1 );
#endif

  return output;
}

