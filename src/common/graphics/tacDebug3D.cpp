#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacMemory.h"
#include "src/common/shell/tacShellTimer.h"

#include <cmath>

namespace Tac
{

  static const int cylinderSegmentCount = 10;
  static const int hemisphereSegmentCount = 4;
  static const int numdivisions = 20;

  static struct Debug3DCommonData
  {
    void Init( Errors& );
    void Uninit();
    Render::BlendStateHandle      mAlphaBlendState;
    Render::ConstantBufferHandle  mCBufferPerFrame;
    Render::DepthStateHandle      mDepthLess;
    Render::RasterizerStateHandle mRasterizerStateNoCull;
    Render::ShaderHandle          m3DVertexColorShader;
    Render::VertexFormatHandle    mVertexColorFormat;
  } gDebug3DCommonData;

  static DefaultCBufferPerFrame Debug3DGetPerFrameData( const Render::ViewHandle viewId,
                                                        const Camera* camera,
                                                        const int viewWidth,
                                                        const int viewHeight )

  {
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                        camera->mNearPlane,
                                        a,
                                        b );

    const float w = ( float )viewWidth;
    const float h = ( float )viewHeight;
    const float aspect = w / h;
    const m4 view = camera->View();
    const m4 proj = camera->Proj( a, b, aspect );

    const double elapsedSeconds = ShellGetElapsedSeconds();

    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = camera->mFarPlane;
    perFrameData.mNear = camera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };
    perFrameData.mSecModTau = ( float )std::fmod( elapsedSeconds, 6.2831853 );
    return perFrameData;
  }

  void Debug3DCommonDataInit( Errors& errors ) { gDebug3DCommonData.Init( errors ); }
  void Debug3DCommonDataUninit() { gDebug3DCommonData.Uninit(); }

  void Debug3DCommonData::Uninit()
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
    mRasterizerStateNoCull = [](){
      Render::RasterizerState rasterizerStateNoCullData;
      rasterizerStateNoCullData.mCullMode = Render::CullMode::None;
      rasterizerStateNoCullData.mFillMode = Render::FillMode::Solid;
      rasterizerStateNoCullData.mFrontCounterClockwise = true;
      rasterizerStateNoCullData.mMultisample = false;
      rasterizerStateNoCullData.mScissor = true;
      auto rast = Render::CreateRasterizerState( rasterizerStateNoCullData,
                                                 TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( rast, "debug-3d-rast" );
      return rast;
    }( );


    mCBufferPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                                     DefaultCBufferPerFrame::shaderRegister,
                                                     TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mCBufferPerFrame, "debug-3d-cbuf-frame" );

    m3DVertexColorShader = Render::CreateShader( Render::ShaderSource::FromPath( "3DDebug" ),
                                                 Render::ConstantBuffers{ mCBufferPerFrame },
                                                 TAC_STACK_FRAME );

    Render::DepthState depthStateData;
    depthStateData.mDepthFunc = Render::DepthFunc::Less;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    mDepthLess = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mCBufferPerFrame, "debug-3d-depth-state" );

    Render::VertexDeclaration positionData;
    positionData.mAttribute = Render::Attribute::Position;
    positionData.mTextureFormat = formatv3;
    positionData.mAlignedByteOffset = TAC_OFFSET_OF( DefaultVertexColor, mPosition );
    Render::VertexDeclaration colorData;
    colorData.mAttribute = Render::Attribute::Color;
    colorData.mTextureFormat = formatv3;
    colorData.mAlignedByteOffset = TAC_OFFSET_OF( DefaultVertexColor, mColor );
    mVertexColorFormat = Render::CreateVertexFormat( { positionData, colorData },
                                                     m3DVertexColorShader,
                                                     TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mVertexColorFormat, "debug-3d-vtx-fmt" );

    Render::BlendState alphaBlendStateData;
    alphaBlendStateData.mSrcRGB = Render::BlendConstants::One;
    alphaBlendStateData.mDstRGB = Render::BlendConstants::OneMinusSrcA;
    alphaBlendStateData.mBlendRGB = Render::BlendMode::Add;
    alphaBlendStateData.mSrcA = Render::BlendConstants::One;
    alphaBlendStateData.mDstA = Render::BlendConstants::OneMinusSrcA;
    alphaBlendStateData.mBlendA = Render::BlendMode::Add;
    mAlphaBlendState = Render::CreateBlendState( alphaBlendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mAlphaBlendState, "debug-3d-alpha-blend" );
  }

  Debug3DDrawData::~Debug3DDrawData()
  {
    if( mVerts.IsValid() )
      Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
  }
  void Debug3DDrawData::DebugDraw3DLine( v3 p0, v3 p1, v3 color0, v3 color1 )
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
  void Debug3DDrawData::DebugDraw3DLine( v3 p0, v3 p1, v3 color )
  {
    DebugDraw3DLine( p0, p1, color, color );
  }
  void Debug3DDrawData::DebugDraw3DCircle( v3 p0, v3 dir, float rad, v3 color )
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
      DebugDraw3DLine( prevPoint, point, color );
      prevPoint = point;
    }
  }
  void Debug3DDrawData::DebugDraw3DSphere( v3 origin, float radius, v3 color )
  {
    DebugDraw3DCircle( origin, v3( 1, 0, 0 ), radius, color );
    DebugDraw3DCircle( origin, v3( 0, 1, 0 ), radius, color );
    DebugDraw3DCircle( origin, v3( 0, 0, 1 ), radius, color );
  }
  void Debug3DDrawData::DebugDraw3DCapsule( v3 p0, v3 p1, float radius, v3 color )
  {
    auto dir = p1 - p0;
    auto dirlen = Length( dir );
    if( dirlen < 0.001f )
    {
      DebugDraw3DSphere( p0, radius, color );
      return;
    }
    dir /= dirlen;
    DebugDraw3DHemisphere( p0, -dir, radius, color );
    DebugDraw3DHemisphere( p1, dir, radius, color );
    DebugDraw3DCylinder( p0, p1, radius, color );
  }
  void Debug3DDrawData::DebugDraw3DHemisphere( v3 mOrigin, v3 mDirection, float radius, v3 color )
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
        DebugDraw3DLine(
          mPrevPts[ iCylinder ],
          mCurrPts[ iCylinder ],
          color );

        int iCylinderNext = ( iCylinder + 1 ) % cylinderSegmentCount;

        DebugDraw3DLine(
          mCurrPts[ iCylinder ],
          mCurrPts[ iCylinderNext ],
          color );
      }
      mPrevPts = mCurrPts;
    }
  }
  void Debug3DDrawData::DebugDraw3DCylinder( v3 p0, v3 p1, float radius, v3 color )
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
      DebugDraw3DLine( p0Points[ iSegment ], p0Points[ iSegmentNext ], color );
      DebugDraw3DLine( p1Points[ iSegment ], p1Points[ iSegmentNext ], color );
      DebugDraw3DLine( p0Points[ iSegment ], p1Points[ iSegment ], color );
    }
  }
  void Debug3DDrawData::DebugDraw3DGrid( v3 lineColor )
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
      DebugDraw3DLine(
        v3( ( float )-extent, 0, ( float )i ),
        v3( ( float )extent, 0, ( float )i ),
        lineColor );
      DebugDraw3DLine(
        v3( ( float )i, 0, ( float )-extent ),
        v3( ( float )i, 0, ( float )extent ),
        lineColor );
    }
  }
  void Debug3DDrawData::DebugDraw3DArrow( v3 from, v3 to, v3 color )
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
    DebugDraw3DLine( from, to, color );
    DebugDraw3DLine( to, circlePos + tan1, color );
    DebugDraw3DLine( to, circlePos - tan1, color );
    DebugDraw3DLine( to, circlePos + tan2, color );
    DebugDraw3DLine( to, circlePos - tan2, color );
  }
  void Debug3DDrawData::DebugDraw3DOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color )
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
        DebugDraw3DLine( transfor_point_i, transfor_point_j, color );
      }
    }
  }
  void Debug3DDrawData::DebugDraw3DAABB( v3 mini, v3 maxi, v3 miniColor, v3 maxiColor )
  {

    for( int a0 = 0; a0 < 3; ++a0 )
    {
      v3 v0;
      v3 v0Color;
      v0[ a0 ] = mini[ a0 ];
      v0Color[ a0 ] = miniColor[ a0 ];

      v3 v1;
      v3 v1Color;
      v1[ a0 ] = maxi[ a0 ];
      v1Color[ a0 ] = maxiColor[ a0 ];
      int a1 = ( a0 + 1 ) % 3;
      int a2 = ( a0 + 2 ) % 3;
      for( float a1Val : { mini[ a1 ], maxi[ a1 ] } )
      {
        v0[ a1 ] = a1Val;
        v1[ a1 ] = a1Val;
        v0Color[ a1 ] = a1Val == mini[ a1 ] ? miniColor[ a1 ] : maxiColor[ a1 ];
        v1Color[ a1 ] = a1Val == mini[ a1 ] ? miniColor[ a1 ] : maxiColor[ a1 ];
        for( float a2Val : { mini[ a2 ], maxi[ a2 ] } )
        {
          v0[ a2 ] = a2Val;
          v1[ a2 ] = a2Val;
          v0Color[ a2 ] = a2Val == mini[ a2 ] ? miniColor[ a2 ] : maxiColor[ a2 ];
          v1Color[ a2 ] = a2Val == mini[ a2 ] ? miniColor[ a2 ] : maxiColor[ a2 ];
          DebugDraw3DLine( v0, v1, v0Color, v1Color );
        }
      }
    }
  }

  void Debug3DDrawData::DebugDraw3DAABB( v3 mini, v3 maxi, v3 color )
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
          DebugDraw3DLine( v0, v1, color );
        }
      }
    }
  }
  void Debug3DDrawData::DebugDraw3DTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 )
  {
    DebugDraw3DLine( p0, p1, color0, color1 );
    DebugDraw3DLine( p0, p2, color0, color2 );
    DebugDraw3DLine( p1, p2, color1, color2 );
  }
  void Debug3DDrawData::DebugDraw3DTriangle( v3 p0, v3 p1, v3 p2, v3 color )
  {
    DebugDraw3DTriangle( p0, p1, p2, color, color, color );
  }
  void Debug3DDrawData::DebugDraw3DToTexture( const Render::ViewHandle viewId,
                                              const Camera* camera,
                                              const int viewWidth,
                                              const int viewHeight,
                                              Errors& errors )
  {
    //_PROFILE_BLOCK;
    const int vertexCount = mDebugDrawVerts.size();
    if( mDebugDrawVerts.size() )
    {
      if( !mVerts.IsValid() || mCapacity < mDebugDrawVerts.size() )
      {
        if( mVerts.IsValid() )
          Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
        mVerts = Render::CreateVertexBuffer( mDebugDrawVerts.size() * sizeof( DefaultVertexColor ),
                                             mDebugDrawVerts.data(),
                                             sizeof( DefaultVertexColor ),
                                             Render::Access::Dynamic,
                                             TAC_STACK_FRAME );
        Render::SetRenderObjectDebugName( mVerts, "debug-3d-vtxes" );

        mCapacity = mDebugDrawVerts.size();
      }
      else
      {
        Render::UpdateVertexBuffer( mVerts,
                                    mDebugDrawVerts.data(),
                                    mDebugDrawVerts.size() * sizeof( DefaultVertexColor ),
                                    TAC_STACK_FRAME );
      }

      mDebugDrawVerts.clear();
    }

    DefaultCBufferPerFrame perFrameData = Debug3DGetPerFrameData( viewId, camera, viewWidth, viewHeight );
    Render::UpdateConstantBuffer( gDebug3DCommonData.mCBufferPerFrame,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    Render::BeginGroup( "debug 3d", TAC_STACK_FRAME );
    Render::SetBlendState( gDebug3DCommonData.mAlphaBlendState );
    Render::SetDepthState( gDebug3DCommonData.mDepthLess );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::LineList );
    Render::SetShader( gDebug3DCommonData.m3DVertexColorShader );
    Render::SetRasterizerState( gDebug3DCommonData.mRasterizerStateNoCull );
    Render::SetVertexFormat( gDebug3DCommonData.mVertexColorFormat );
    Render::SetVertexBuffer( mVerts, 0, vertexCount );
    Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
    Render::Submit( viewId, TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }

}

