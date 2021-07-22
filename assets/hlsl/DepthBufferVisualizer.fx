Texture2D depthbuf    : register( t0 );
sampler   pointSamp   : register( s0 );
cbuffer   CBufferVizf : register( b0 )
{
  float projA;
  float projB;
  float f;
  float n;
};

float UnprojectNDCToView( float zNDC, float a, float b )
{
  return -b / ( a + zNDC );
}


float2 FullScreenTriangleUV( uint i )
{
  float2 opengl_uvs[ 3 ] =
  {
    float2( 0, 0 ),
    float2( 2, 0 ),
    float2( 0, 2 ),
  };
  float2 directx_uvs[ 3 ] =
  {
    float2( 0, 1 ),
    float2( 2, 1 ),
    float2( 0, -1 ),
  };
  return directx_uvs[ i ];
}

float4 FullScreenTrianglePosition( uint i )
{
  float4 clipSpacePositions[ 3 ] =
  {
    float4( -1, -1, 0, 1 ),
    float4( 3, -1, 0, 1 ),
    float4( -1, 3, 0, 1 ),
  };
  return clipSpacePositions[ i ];
}

//===------------ Vertex Shader ------------===//

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 mUV                : SV_AUTO_SEMANTIC;
};


VS_OUTPUT VS( const uint iVtx : SV_VertexID )
{
  VS_OUTPUT output;
  output.mClipSpacePosition = FullScreenTrianglePosition( iVtx );
  output.mUV = FullScreenTriangleUV( iVtx );
  return output;
}

//===------------ Pixel Shader ------------===//

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  float ndc = depthbuf.Sample( pointSamp, input.mUV ).x;
  float view = UnprojectNDCToView( ndc, projA, projB );
  float linearizedNormalized = ( -view - n ) / ( f - n );

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor = float4( float3( 1, 1, 1 ) * linearizedNormalized, 1 );

  return output;
}

