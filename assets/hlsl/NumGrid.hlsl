#pragma pack_matrix( row_major )

struct NumGridCBufData
{
  row_major matrix mClipFromModel;
  uint   mWidth;
  uint   mHeight;
};

ConstantBuffer< NumGridCBufData > sNumGrid        : TAC_AUTO_REGISTER;
Buffer< uint >                    sTextureIndices : TAC_AUTO_REGISTER;
Texture2D                         sTextures[]     : TAC_AUTO_REGISTER;
SamplerState                      sSampler        : TAC_AUTO_REGISTER;

struct VS_INPUT
{
  uint mVertexID : SV_VertexID;
};

struct VS_OUTPUT
{
  float2 mUV_directx    : TAC_AUTO_SEMANTIC;
  uint   mGridVal       : TAC_AUTO_SEMANTIC;
  float4 mPos_clipspace : SV_POSITION;
};

VS_OUTPUT VS( VS_INPUT input )
{
  uint iCell = input.mVertexID / 6;
  uint iVert = input.mVertexID % 6;

  float2 uv_BL = float2(0,0);
  float2 uv_BR = float2(1,0);
  float2 uv_TL = float2(0,1);
  float2 uv_TR = float2(1,1);
  float2 uvs[6] = 
  {
    uv_BL, uv_BR, uv_TL,
    uv_TL, uv_BR, uv_TR,
  };

  float2 uv = uvs[ iVert ]; // opengl style

  uint row = iCell / sNumGrid.mWidth;
  uint col = iCell % sNumGrid.mWidth;

  float4 pos_modelspace = float4( uv.x + col, 0, -uv.y - row, 1 );
  float4 pos_clipspace = mul( sNumGrid.mClipFromModel, pos_modelspace );

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mUV_directx = float2( uv.x, 1.0 - uv.y );
  output.mGridVal = sTextureIndices[iCell];
  output.mPos_clipspace = pos_clipspace;
  return output;
}

// -----------------------------------------------------------------

struct PS_INPUT
{
  float2 mUV_directx : TAC_AUTO_SEMANTIC;
  uint   mGridVal    : TAC_AUTO_SEMANTIC;
};

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( PS_INPUT input )
{
  float4 texSample = float4(1,0,0,1);
  if( input.mGridVal != uint(-1) )
    texSample = sTextures[ 
NonUniformResourceIndex(
    input.mGridVal ) ].Sample( sSampler, input.mUV_directx );

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor += texSample;
  //output.mColor.a = 1.;

#if 0
  output.mColor.xyz *= 0.0001;
  texSample *= 0.0001;
  if( input.mGridVal == 4u )
    texSample = sTextures[ input.mGridVal ].Sample( sSampler, input.mUV_directx );
  if( input.mGridVal == 7u )
    texSample += sTextures[ input.mGridVal ].Sample( sSampler, input.mUV_directx );
  if( input.mGridVal == 5u )
    texSample += sTextures[ input.mGridVal ].Sample( sSampler, input.mUV_directx );
  if( input.mGridVal == 6u )
    texSample += sTextures[ input.mGridVal ].Sample( sSampler, input.mUV_directx );
  output.mColor.xyz += texSample.xyz;
  output.mColor.w =1.;
#endif


  return output;
}
