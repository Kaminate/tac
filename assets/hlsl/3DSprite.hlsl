struct PerFrameType
{
  row_major matrix mView;
  row_major matrix mProj;
};

struct PerObjType
{
  row_major matrix mWorld;
};

typedef ConstantBuffer< PerFrameType > PerFrameBuf;
typedef ConstantBuffer< PerObjType >   PerObjBuf;

PerFrameBuf  perFrame      : TAC_AUTO_REGISTER( b );
PerObjBuf    perObj        : TAC_AUTO_REGISTER( b );
Texture2D    sprite        : TAC_AUTO_REGISTER;
SamplerState linearSampler : TAC_AUTO_REGISTER;

struct VS_INPUT
{
  uint iVertex : SV_VERTEXID;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 mDXTexCoord        : TAC_AUTO_SEMANTIC;
};

const float2 GetQuadPos( uint iVertex )
{
  float2 bl = float2( -1, -1 );
  float2 br = float2( 1, -1 );
  float2 tl = float2( -1, 1 );
  float2 tr = float2( 1, 1 );
  switch( iVertex )
  {
    case 0: return br; // tri 0
    case 1: return tr;
    case 2: return tl;
    case 3: return br; // tri 1
    case 4: return tl;
    case 5: return bl;
  }
  return float2( 0, 0 );
}

const float2 GetQuadDXTexCoord( uint iVertex )
{
  float2 bl = float2( 0, 1 );
  float2 br = float2( 1, 1 );
  float2 tl = float2( 0, 0 );
  float2 tr = float2( 1, 0 );
  switch( iVertex )
  {
    case 0: return br; // tri 0
    case 1: return tr;
    case 2: return tl;
    case 3: return br; // tri 1
    case 4: return tl;
    case 5: return bl;
  }
  return float2( 0, 0 );
}

VS_OUTPUT VS( VS_INPUT input )
{
  matrix world = perObj.mWorld;
  matrix view = perFrame.mView;
  matrix proj = perFrame.mProj;

  float w;
  float h;
  sprite.GetDimensions( w, h );

  const float aspect = w / h;
  const float scale = world[ 0 ][ 0 ];

  const float2 quadPos = GetQuadPos( input.iVertex );
  const float2 quadDXTexCoord = GetQuadDXTexCoord( input.iVertex );

  const float4 worldSpacePosition = mul( world, float4( 0, 0, 0, 1 ) );
  const float4 viewSpacePosition
    = mul( view, worldSpacePosition )
    + float4( quadPos.x * scale * aspect,
              quadPos.y * scale,
              0,
              0 );
  const float4 clipSpacePosition = mul( proj, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mDXTexCoord = quadDXTexCoord;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  float4 sampled = sprite.Sample( linearSampler, input.mDXTexCoord );

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor = sampled;

  return output;
}

