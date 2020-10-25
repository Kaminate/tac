#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacMemory.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/profile/tacProfile.h"
namespace Tac
{

  static const int cylinderSegmentCount = 10;
  static const int hemisphereSegmentCount = 4;
  static const int numdivisions = 20;

  Debug3DCommonData* Debug3DCommonData::Instance = nullptr;
  Debug3DCommonData::Debug3DCommonData()
  {
    Instance = this;
  }
  Debug3DCommonData::~Debug3DCommonData()
  {
    Render::DestroyBlendState( mAlphaBlendState, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mCBufferPerFrame, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthLess, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerStateNoCull, TAC_STACK_FRAME );
    Render::DestroyShader( m3DVertexColorShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( mVertexColorFormat, TAC_STACK_FRAME );
  }
  void Debug3DCommonData::Init( Errors& errors )
  {
    Render::RasterizerState rasterizerStateNoCullData;
    rasterizerStateNoCullData.mCullMode = CullMode::None;
    rasterizerStateNoCullData.mFillMode = FillMode::Solid;
    rasterizerStateNoCullData.mFrontCounterClockwise = true;
    rasterizerStateNoCullData.mMultisample = false;
    rasterizerStateNoCullData.mScissor = true;
    mRasterizerStateNoCull = Render::CreateRasterizerState( "no cull",
                                                            rasterizerStateNoCullData,
                                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );


    mCBufferPerFrame = Render::CreateConstantBuffer( "cbuffer per frame",
                                                     sizeof( DefaultCBufferPerFrame ),
                                                     DefaultCBufferPerFrame::shaderRegister,
                                                     TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    m3DVertexColorShader = Render::CreateShader( "3d color",
                                                 Render::ShaderSource::FromPath( "3DDebug" ),
                                                 Render::ConstantBuffers( mCBufferPerFrame ),
                                                 TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthFunc = DepthFunc::Less;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    mDepthLess = Render::CreateDepthState( "depth less", depthStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration positionData;
    positionData.mAttribute = Attribute::Position;
    positionData.mTextureFormat = formatv3;
    positionData.mAlignedByteOffset = TAC_OFFSET_OF( DefaultVertexColor, mPosition );
    VertexDeclaration colorData;
    colorData.mAttribute = Attribute::Color;
    colorData.mTextureFormat = formatv3;
    colorData.mAlignedByteOffset = TAC_OFFSET_OF( DefaultVertexColor, mColor );

    Render::VertexDeclarations vertexDeclarations;
    vertexDeclarations.AddVertexDeclaration( positionData );
    vertexDeclarations.AddVertexDeclaration( colorData );
    mVertexColorFormat = Render::CreateVertexFormat( "vertex color format",
                                                     vertexDeclarations,
                                                     m3DVertexColorShader,
                                                     TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::BlendState alphaBlendStateData;
    alphaBlendStateData.srcRGB = BlendConstants::One;
    alphaBlendStateData.dstRGB = BlendConstants::OneMinusSrcA;
    alphaBlendStateData.blendRGB = BlendMode::Add;
    alphaBlendStateData.srcA = BlendConstants::One;
    alphaBlendStateData.dstA = BlendConstants::OneMinusSrcA;
    alphaBlendStateData.blendA = BlendMode::Add;
    mAlphaBlendState = Render::CreateBlendState( "alpha blend", alphaBlendStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  //DebugDrawAABB DebugDrawAABB::FromMinMax( v3 mini, v3 maxi )
  //{
  //  DebugDrawAABB result;
  //  result.mMini = mini;
  //  result.mMaxi = maxi;
  //  return result;
  //}
  //DebugDrawAABB DebugDrawAABB::FromPosExtents( v3 pos, v3 extents )
  //{
  //  DebugDrawAABB result;
  //  result.mMini = pos - extents;
  //  result.mMaxi = pos + extents;
  //  return result;
  //}

  Debug3DDrawData::~Debug3DDrawData()
  {
    if( mVerts.IsValid() )
      Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
  }
  void Debug3DDrawData::DebugDrawLine( v3 p0, v3 p1, v3 color0, v3 color1 )
  {
    if( !IsDebugMode() )
      return;
    DefaultVertexColor defVert0;
    defVert0.mColor = color0;
    defVert0.mPosition = p0;

    DefaultVertexColor defVert1;
    defVert1.mColor = color1;
    defVert1.mPosition = p1;

    mDebugDrawVerts.push_back( defVert0 );
    mDebugDrawVerts.push_back( defVert1 );
  }
  void Debug3DDrawData::DebugDrawLine( v3 p0, v3 p1, v3 color )
  {
    DebugDrawLine( p0, p1, color, color );
  }
  void Debug3DDrawData::DebugDrawCircle( v3 p0, v3 dir, float rad, v3 color )
  {
    auto length = Length( dir );
    if( length < 0.001f || std::abs( rad ) < 0.001f )
      return;
    dir /= length;
    v3 tan1;
    v3 tan2;
    GetFrameRH( dir, tan1, tan2 );
    tan1 *= rad;
    tan2 *= rad;
    v3 prevPoint = p0 + tan1;
    for( int i = 1; i <= numdivisions; ++i )
    {
      float theta = 3.14f * 2.0f * ( float )i / ( float )numdivisions;
      v3 point
        = p0
        + std::cos( theta ) * tan1
        + std::sin( theta ) * tan2;
      DebugDrawLine( prevPoint, point, color );
      prevPoint = point;
    }
  }
  void Debug3DDrawData::DebugDrawSphere( v3 origin, float radius, v3 color )
  {
    DebugDrawCircle( origin, v3( 1, 0, 0 ), radius, color );
    DebugDrawCircle( origin, v3( 0, 1, 0 ), radius, color );
    DebugDrawCircle( origin, v3( 0, 0, 1 ), radius, color );
  }
  void Debug3DDrawData::DebugDrawCapsule( v3 p0, v3 p1, float radius, v3 color )
  {
    auto dir = p1 - p0;
    auto dirlen = Length( dir );
    if( dirlen < 0.001f )
    {
      DebugDrawSphere( p0, radius, color );
      return;
    }
    dir /= dirlen;
    DebugDrawHemisphere( p0, -dir, radius, color );
    DebugDrawHemisphere( p1, dir, radius, color );
    DebugDrawCylinder( p0, p1, radius, color );
  }
  void Debug3DDrawData::DebugDrawHemisphere( v3 mOrigin, v3 mDirection, float radius, v3 color )
  {
    Vector< v3 > mPrevPts( cylinderSegmentCount );
    Vector< v3 > mCurrPts( cylinderSegmentCount );
    float hemisphereRads = 0;
    float dHemisphereRads = ( 3.14f * 0.5f ) / hemisphereSegmentCount;
    v3 tan1;
    v3 tan2;
    GetFrameRH( mDirection, tan1, tan2 );
    for( int iCylinder = 0; iCylinder < cylinderSegmentCount; ++iCylinder )
      mPrevPts[ iCylinder ] = mOrigin + mDirection * radius;
    for( int iHemisphere = 0; iHemisphere < hemisphereSegmentCount; ++iHemisphere )
    {
      hemisphereRads += dHemisphereRads;
      float circleRadius = radius * std::sin( hemisphereRads );
      float circleOffset = radius * std::cos( hemisphereRads );
      v3 circleOrigin = mOrigin + mDirection * circleOffset;

      float circleRads = 0;
      float dCircleRads = ( 3.14f * 2.0f ) / cylinderSegmentCount;
      for( int iCylinder = 0; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        mCurrPts[ iCylinder ]
          = circleOrigin
          + tan1 * circleRadius * std::cos( circleRads )
          + tan2 * circleRadius * std::sin( circleRads );
        circleRads += dCircleRads;
      }
      for( int iCylinder = 0; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        DebugDrawLine(
          mPrevPts[ iCylinder ],
          mCurrPts[ iCylinder ],
          color );

        int iCylinderNext = ( iCylinder + 1 ) % cylinderSegmentCount;

        DebugDrawLine(
          mCurrPts[ iCylinder ],
          mCurrPts[ iCylinderNext ],
          color );
      }
      mPrevPts = mCurrPts;
    }
  }
  void Debug3DDrawData::DebugDrawCylinder( v3 p0, v3 p1, float radius, v3 color )
  {
    auto dir = p1 - p0;
    auto dirlen = Length( dir );
    if( dirlen < 0.001f )
      return;
    dir /= dirlen;
    Vector< v3 > p0Points( cylinderSegmentCount );
    Vector< v3 > p1Points( cylinderSegmentCount );
    v3 tan1;
    v3 tan2;
    GetFrameRH( dir, tan1, tan2 );
    tan1 *= radius;
    tan2 *= radius;
    for( int iSegment = 0; iSegment < cylinderSegmentCount; ++iSegment )
    {
      auto angle = ( 3.14f * 2.0f ) * iSegment / ( float )cylinderSegmentCount;
      auto s = std::sin( angle );
      auto c = std::cos( angle );
      auto offset = c * tan1 + s * tan2;
      p0Points[ iSegment ] = p0 + offset;
      p1Points[ iSegment ] = p1 + offset;
    }
    for( int iSegment = 0; iSegment < cylinderSegmentCount; ++iSegment )
    {
      int iSegmentNext = ( iSegment + 1 ) % cylinderSegmentCount;
      DebugDrawLine( p0Points[ iSegment ], p0Points[ iSegmentNext ], color );
      DebugDrawLine( p1Points[ iSegment ], p1Points[ iSegmentNext ], color );
      DebugDrawLine( p0Points[ iSegment ], p1Points[ iSegment ], color );
    }
  }
  void Debug3DDrawData::DebugDrawGrid( v3 lineColor )
  {
    const int extent = 10;
    for( int i = -extent; i <= extent; ++i )
    {
      //if( i == 0 )
      //{
      //  for( int axis = 0; axis < 3; ++axis )
      //  {
      //    v3 to = {};
      //    to[ axis ] = ( float )extent;
      //    if( axis != 1 )
      //      DebugDrawLine( v3(), -to, lineColor );
      //    v3 arrowColor = {};
      //    arrowColor[ axis ] = 1;
      //    DebugDrawArrow( v3(), to, arrowColor );
      //  }
      //  continue;
      //}
      // since we have y up, draw on the xz plane
      DebugDrawLine(
        v3( ( float )-extent, 0, ( float )i ),
        v3( ( float )extent, 0, ( float )i ),
        lineColor );
      DebugDrawLine(
        v3( ( float )i, 0, ( float )-extent ),
        v3( ( float )i, 0, ( float )extent ),
        lineColor );
    }
  }
  void Debug3DDrawData::DebugDrawArrow( v3 from, v3 to, v3 color )
  {
    auto arrowDiff = to - from;
    auto arrowDiffLenSq = Quadrance( arrowDiff );
    if( arrowDiffLenSq < 0.001f )
      return;
    auto arrowDiffLen = std::sqrt( arrowDiffLenSq );
    auto arrowDir = arrowDiff / arrowDiffLen;

    auto arrowHeadLen = arrowDiffLen * 0.2f;
    auto arrowHeadRadius = arrowHeadLen * 0.4f;

    v3 tan1, tan2;
    GetFrameRH( arrowDir, tan1, tan2 );
    tan1 *= arrowHeadRadius;
    tan2 *= arrowHeadRadius;
    auto circlePos = to - arrowDir * arrowHeadLen;
    DebugDrawLine( from, to, color );
    DebugDrawLine( to, circlePos + tan1, color );
    DebugDrawLine( to, circlePos - tan1, color );
    DebugDrawLine( to, circlePos + tan2, color );
    DebugDrawLine( to, circlePos - tan2, color );
  }
  void Debug3DDrawData::DebugDrawOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color )
  {
    const int pointCount = 8;
    v3 original_points[] = {
      v3( -1, -1, -1 ),
      v3( -1, -1, 1 ),
      v3( -1, 1, -1 ),
      v3( -1, 1, 1 ),
      v3( 1, -1, -1 ),
      v3( 1, -1, 1 ),
      v3( 1, 1, -1 ),
      v3( 1, 1, 1 ) };
    v3 transfor_points[ pointCount ];
    auto transform = m4::Transform( extents, eulerAnglesRad, pos );
    for( int i = 0; i < pointCount; ++i )
      transfor_points[ i ] = ( transform * v4( original_points[ i ], 1.0f ) ).xyz();
    for( int i = 0; i < pointCount; ++i )
    {
      const v3& original_point_i = original_points[ i ];
      const v3& transfor_point_i = transfor_points[ i ];
      for( int j = i + 1; j < pointCount; ++j )
      {
        const v3& original_point_j = original_points[ j ];
        const v3& transfor_point_j = transfor_points[ j ];

        int numSame = 0;
        for( int axis = 0; axis < 3; ++axis )
          if( original_point_i[ axis ] == original_point_j[ axis ] )
            numSame++;
        if( numSame != 2 )
          continue;
        DebugDrawLine( transfor_point_i, transfor_point_j, color );
      }
    }
  }
  void Debug3DDrawData::DebugDrawAABB( v3 mini, v3 maxi, v3 color )
  {
    for( int a0 = 0; a0 < 3; ++a0 )
    {
      v3 v0;
      v0[ a0 ] = mini[ a0 ];

      v3 v1;
      v1[ a0 ] = maxi[ a0 ];
      int a1 = ( a0 + 1 ) % 3;
      int a2 = ( a0 + 2 ) % 3;
      for( float a1Val : { mini[ a1 ], maxi[ a1 ] } )
      {
        v0[ a1 ] = v1[ a1 ] = a1Val;
        for( float a2Val : { mini[ a2 ], maxi[ a2 ] } )
        {
          v0[ a2 ] = v1[ a2 ] = a2Val;
          DebugDrawLine( v0, v1, color );
        }
      }
    }
  }
  void Debug3DDrawData::DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 )
  {
    DebugDrawLine( p0, p1, color0, color1 );
    DebugDrawLine( p0, p2, color0, color2 );
    DebugDrawLine( p1, p2, color1, color2 );
  }
  void Debug3DDrawData::DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color )
  {
    DebugDrawTriangle( p0, p1, p2, color, color, color );
  }
  void Debug3DDrawData::DrawToTexture( Errors& errors,
    const DefaultCBufferPerFrame* cbufferperframe,
    RenderView* renderView )
  {
    //_PROFILE_BLOCK;
    if( mDebugDrawVerts.size() )
    {
      if( !mVerts.IsValid() || mCapacity < mDebugDrawVerts.size() )
      {
        if( mVerts.IsValid() )
          Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
        mVerts = Render::CreateVertexBuffer( "debug 3d verts",
                                             mDebugDrawVerts.size() * sizeof( DefaultVertexColor ),
                                             mDebugDrawVerts.data(),
                                             0,
                                             Access::Dynamic,
                                             TAC_STACK_FRAME );
        TAC_HANDLE_ERROR( errors );

        mCapacity = mDebugDrawVerts.size();
      }
      else
      {
        Render::UpdateVertexBuffer( mVerts,
                                    mDebugDrawVerts.data(),
                                    mDebugDrawVerts.size() * sizeof( DefaultVertexColor ),
                                    TAC_STACK_FRAME );
      }

      //DrawCall2 drawCall;
      //drawCall.mBlendState = Debug3DCommonData::Instance->mAlphaBlendState;
      //drawCall.mDepthState = Debug3DCommonData::Instance->mDepthLess;
      //drawCall.mIndexCount = 0;
      //drawCall.mVertexCount = mDebugDrawVerts.size();
      //drawCall.mPrimitiveTopology = PrimitiveTopology::LineList;
      //drawCall.mRasterizerState = Debug3DCommonData::Instance->mRasterizerStateNoCull;
      //drawCall.mShader = Debug3DCommonData::Instance->m3DVertexColorShader;
      //drawCall.mFrame = TAC_STACK_FRAME;
      //drawCall.mStartIndex = 0;
      //drawCall.mUniformDst = Debug3DCommonData::Instance->mCBufferPerFrame;
      //drawCall.CopyUniformSource( *cbufferperframe );
      //drawCall.mVertexBuffer = mVerts;
      //drawCall.mVertexFormat = Debug3DCommonData::Instance->mVertexColorFormat;
      //Render::AddDrawCall( drawCall );

      mDebugDrawVerts.clear();
    }

    //Renderer::Instance->DebugBegin( "debug 3d" );
    //Renderer::Instance->RenderFlush();
    //Renderer::Instance->DebugEnd();
  }

}

