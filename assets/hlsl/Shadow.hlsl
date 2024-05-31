struct ShadowFrameConstantsType
{
  row_major matrix mView;
  row_major matrix mProjection;
};

struct ShadowObjectConstantsType
{
  row_major matrix mWorld;
}

typedef ShadowConstantBuffer< ShadowFrameConstantsType > ShadowPerFrame;
typedef ShadowConstantBuffer< ShadowObjectConstantsType > ShadowPerObj;

ShadowPerFrame sPerFrame : register( b0 );
ShadowPerObj   sPerObj   : register( b1 );

struct VS_INPUT
{
  float3 Position           : POSITION;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float4 debug_view_pos     : TAC_AUTO_SEMANTIC;
  float3 debug_ndc_pos      : TAC_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( sPerObj.mWorld, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( sPerFrame.mView, worldSpacePosition );
  float4 clipSpacePosition = mul( sPerFrame.mProjection, viewSpacePosition );

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.debug_view_pos = viewSpacePosition;
  output.debug_ndc_pos = clipSpacePosition.xyz / clipSpacePosition.w;
  return output;
}

void PS( VS_OUTPUT input )
{
  // do nothing, we only care about the value written to the depth buffer
}

