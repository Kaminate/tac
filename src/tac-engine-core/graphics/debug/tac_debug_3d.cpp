#include "tac_debug_3d.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"

//#include <cmath>

namespace Tac
{

  static const int cylinderSegmentCount = 10;
  static const int hemisphereSegmentCount = 4;
  static const int numdivisions = 20;

  static struct Debug3DCommonData
  {
    void                          Init( Errors& );
    void                          Uninit();
#if TAC_TEMPORARILY_DISABLED()
    Render::BlendStateHandle      mAlphaBlendState;
    Render::DepthStateHandle      mDepthLess;
    Render::RasterizerStateHandle mRasterizerStateNoCull;
    Render::ProgramHandle         m3DVertexColorShader;
    Render::VertexFormatHandle    mVertexColorFormat;
#endif
  } gDebug3DCommonData;

  static Render::DefaultCBufferPerFrame Debug3DGetPerFrameData( const Render::TextureHandle viewId,
                                                                const Camera* camera,
                                                                const int viewWidth,
                                                                const int viewHeight )

  {
    const Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    const auto ndcAttribs = renderDevice->GetInfo().mNDCAttribs;
    const m4::ProjectionMatrixParams projMtxParams
    {
      .mNDCMinZ = ndcAttribs.mMinZ,
      .mNDCMaxZ = ndcAttribs.mMaxZ,
      .mViewSpaceNear = camera->mNearPlane,
      .mViewSpaceFar = camera->mFarPlane,
      .mAspectRatio = ( float )viewWidth / ( float )viewHeight,
      .mFOVYRadians = camera->mFovyrad,
    };

    const m4 view = camera->View();
    const m4 proj = m4::ProjPerspective( projMtxParams );
    const Timestamp elapsedSeconds = Timestep::GetElapsedTime();
    return Render::DefaultCBufferPerFrame
    {
      .mView = view,
      .mProjection = proj,
      .mFar = camera->mFarPlane,
      .mNear = camera->mNearPlane,
      .mGbufferSize = { ( float )viewWidth, ( float )viewHeight },
      .mSecModTau = ( float )Fmod( elapsedSeconds.mSeconds, 6.2831853 ),
    };
  }

  void Debug3DCommonDataInit( Errors& errors ) { gDebug3DCommonData.Init( errors ); }

  void Debug3DCommonDataUninit() { gDebug3DCommonData.Uninit(); }

  void Debug3DCommonData::Uninit()
  {
#if TAC_TEMPORARILY_DISABLED()
    Render::DestroyBlendState( mAlphaBlendState, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthLess, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerStateNoCull, TAC_STACK_FRAME );
    Render::DestroyProgram( m3DVertexColorShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( mVertexColorFormat, TAC_STACK_FRAME );
#endif
  }

  void Debug3DCommonData::Init( Errors& errors )
  {
#if TAC_TEMPORARILY_DISABLED()
    const Render::RasterizerState rasterizerState
    {
      .mFillMode = Render::FillMode::Solid,
      .mCullMode = Render::CullMode::None,
      .mFrontCounterClockwise = true,
      .mScissor = true,
      .mMultisample = false, 
    };
    mRasterizerStateNoCull = Render::CreateRasterizerState( rasterizerState,
                                                            TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerStateNoCull, "debug-3d-rast" );


    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::ProgramParams programParams
    {
      .mFileStem = "3DDebug",
      .mStackFrame = TAC_STACK_FRAME,
    };
    m3DVertexColorShader = renderDevice->CreateProgram( programParams, errors );

    const Render::DepthState depthStateData
    {
      .mDepthTest = true,
      .mDepthWrite = true,
      .mDepthFunc = Render::DepthFunc::Less
    };
    mDepthLess = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthLess, "debug-3d-depth-state" );

    const Render::VertexDeclaration posDecl
    {
      .mAttribute = Render::Attribute::Position,
      .mTextureFormat
      {
        .mElementCount = 3,
        .mPerElementByteCount = sizeof( float ),
        .mPerElementDataType = Render::GraphicsType::real
      },
      .mAlignedByteOffset = (int)TAC_OFFSET_OF( DefaultVertexColor, mPosition )
    } ;

    const Render::VertexDeclaration colDecl
    {
      .mAttribute = Render::Attribute::Color,
      .mTextureFormat
      {
        .mElementCount = 4,
        .mPerElementByteCount = sizeof( float ),
        .mPerElementDataType = Render::GraphicsType::real
      },
      .mAlignedByteOffset = (int)TAC_OFFSET_OF( DefaultVertexColor, mColor )
    } ;

    Render::VertexDeclarations decls;
    decls.push_back( posDecl );
    decls.push_back( colDecl );

    mVertexColorFormat = Render::CreateVertexFormat( decls, m3DVertexColorShader, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mVertexColorFormat, "debug-3d-vtx-fmt" );

    const Render::BlendState alphaBlendStateData{ .mSrcRGB = Render::BlendConstants::SrcA,
                                                  .mDstRGB = Render::BlendConstants::OneMinusSrcA,
                                                  .mBlendRGB = Render::BlendMode::Add,
                                                  .mSrcA = Render::BlendConstants::Zero,
                                                  .mDstA = Render::BlendConstants::One,
                                                  .mBlendA = Render::BlendMode::Add,};
    mAlphaBlendState = Render::CreateBlendState( alphaBlendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mAlphaBlendState, "debug-3d-alpha-blend" );
#endif
  }

  Debug3DDrawData::~Debug3DDrawData()
  {
#if TAC_TEMPORARILY_DISABLED()
    if( mVerts.IsValid() )
      Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
#endif
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1 )
  {
    DebugDraw3DLine(p0,p1, v4(1,1,1,1));
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1,
                                         const v3& color0,
                                         const v3& color1 )
  {
    if constexpr( !IsDebugMode )
      return;

    mDebugDrawVerts.push_back( { .mPosition = p0, .mColor = v4( color0, 1.0f ) } );
    mDebugDrawVerts.push_back( { .mPosition = p1, .mColor = v4( color1, 1.0f ) } );
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1,
                                         const v4& color )
  {
    DebugDraw3DLine( p0, p1, color, color );
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1,
                                         const v4& color0,
                                         const v4& color1 )
  {
    if constexpr( !IsDebugMode )
      return;

    mDebugDrawVerts.push_back( { .mPosition = p0, .mColor = color0, } );
    mDebugDrawVerts.push_back( { .mPosition = p1, .mColor = color1, } );
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1,
                                         const v3& color )
  {
    DebugDraw3DLine( p0, p1, color, color );
  }

  void Debug3DDrawData::DebugDraw3DCircle( const v3& p0,
                                           const v3& dir,
                                           float rad,
                                           const v3& color )
  {
    float length = Length( dir );
    if( length < 0.001f || Abs( rad ) < 0.001f )
      return;
    const v3 unitDir = dir / length;
    v3 tan1;
    v3 tan2;
    GetFrameRH( unitDir, tan1, tan2 );
    tan1 *= rad;
    tan2 *= rad;
    v3 prevPoint = p0 + tan1;
    for( int i = 1; i <= numdivisions; ++i )
    {
      float theta = 3.14f * 2.0f * ( float )i / ( float )numdivisions;
      v3 point
        = p0
        + Cos( theta ) * tan1
        + Sin( theta ) * tan2;
      DebugDraw3DLine( prevPoint, point, color );
      prevPoint = point;
    }
  }

  void Debug3DDrawData::DebugDraw3DSphere( const v3& origin,
                                           float radius,
                                           const v3& color )
  {
    DebugDraw3DCircle( origin, v3( 1, 0, 0 ), radius, color );
    DebugDraw3DCircle( origin, v3( 0, 1, 0 ), radius, color );
    DebugDraw3DCircle( origin, v3( 0, 0, 1 ), radius, color );
  }

  void Debug3DDrawData::DebugDraw3DCapsule( const v3& p0,
                                            const v3& p1,
                                            float radius,
                                            const v3& color )
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

  void Debug3DDrawData::DebugDraw3DHemisphere( const v3& mOrigin,
                                               const v3& mDirection,
                                               float radius,
                                               const v3& color )
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
      float circleRadius = radius * Sin( hemisphereRads );
      float circleOffset = radius * Cos( hemisphereRads );
      v3 circleOrigin = mOrigin + mDirection * circleOffset;

      float circleRads = 0;
      float dCircleRads = ( 3.14f * 2.0f ) / cylinderSegmentCount;
      for( int iCylinder = 0; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        mCurrPts[ iCylinder ]
          = circleOrigin
          + tan1 * circleRadius * Cos( circleRads )
          + tan2 * circleRadius * Sin( circleRads );
        circleRads += dCircleRads;
      }
      for( int iCylinder = 0; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        const int iCylinderNext = ( iCylinder + 1 ) % cylinderSegmentCount;
        DebugDraw3DLine( mPrevPts[ iCylinder ], mCurrPts[ iCylinder ], color );
        DebugDraw3DLine( mCurrPts[ iCylinder ], mCurrPts[ iCylinderNext ], color );
      }
      mPrevPts = mCurrPts;
    }
  }

  void Debug3DDrawData::DebugDraw3DCylinder( const v3& p0,
                                             const v3& p1,
                                             float radius,
                                             const v3& color )
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
      auto s = Sin( angle );
      auto c = Cos( angle );
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

  void Debug3DDrawData::DebugDraw3DGrid( const v3& lineColor )
  {
    float d = 10.0f; // Fade distance
    const v4 f( lineColor, 0.0f ); // Far Color
    for( float i = -d; i <= d; i += 1.0f )
    {
      float a = 1 - ( Abs( i ) / d );
      v4 n( lineColor, a ); // Near Color
#if 1 // rainbow
      n.x += a * 1.0f * ( ( Sin( ( float )Timestep::GetElapsedTime() * 3.0f + i ) ) * 0.5f + 0.5f );
      n.y += a * 1.0f * ( ( Sin( ( float )Timestep::GetElapsedTime() * 5.0f + i ) ) * 0.5f + 0.5f );
      n.z += a * 1.0f * ( ( Sin( ( float )Timestep::GetElapsedTime() * 7.0f + i ) ) * 0.5f + 0.5f );
#endif
      if( i )
      {
        DebugDraw3DLine( { -d, 0, i }, { 0, 0, i }, f, n );
        DebugDraw3DLine( { 0, 0, i }, { d, 0, i }, n, f );
        DebugDraw3DLine( { i, 0, -d }, { i, 0, 0 }, f, n );
        DebugDraw3DLine( { i, 0, 0 }, { i, 0, d }, n, f );
      }
    }
    DebugDraw3DLine( { -d, 0, 0 }, { 0, 0, 0 }, f, { lineColor, 1 } );
    DebugDraw3DLine( { 0, 0, -d }, { 0, 0, 0 }, f, { lineColor, 1 } );
    DebugDraw3DLine( {}, { d, 0, 0 }, { 1, 0, 0, 1 }, { 1, 0, 0, 0 } );
    DebugDraw3DLine( {}, { 0, d, 0 }, { 0, 1, 0, 1 }, { 0, 1, 0, 0 } );
    DebugDraw3DLine( {}, { 0, 0, d }, { 0, 0, 1, 1 }, { 0, 0, 1, 0 } );
  }

  void Debug3DDrawData::DebugDraw3DArrow( const v3& from,
                                          const v3& to,
                                          const v3& color )
  {
    auto arrowDiff = to - from;
    auto arrowDiffLenSq = Quadrance( arrowDiff );
    if( arrowDiffLenSq < 0.001f )
      return;
    auto arrowDiffLen = Sqrt( arrowDiffLenSq );
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

  void Debug3DDrawData::DebugDraw3DOBB( const v3& pos,
                                        const v3& halfextents,
                                        const m3& orientation,
                                        const v3& color )
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
    auto transform = m4::Transform( halfextents, orientation, pos );
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

  void Debug3DDrawData::DebugDraw3DOBB( const v3& pos,
                                        const v3& halfextents,
                                        const v3& eulerAnglesRad,
                                        const v3& color )
  {
    m3 orientation = m3::RotRadEuler(eulerAnglesRad);
    DebugDraw3DOBB( pos, halfextents, orientation, color );
  }

  void Debug3DDrawData::DebugDraw3DCube( const v3& pos,
                                         float fullwidth,
                                         const m3& orientation,
                                         const v3& color )
  {
    DebugDraw3DOBB( pos, v3( fullwidth / 2 ), orientation, color );
  }

  void Debug3DDrawData::DebugDraw3DAABB( const v3& mini,
                                         const v3& maxi,
                                         const v3& miniColor,
                                         const v3& maxiColor )
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

  void Debug3DDrawData::DebugDraw3DAABB( const v3& mini,
                                         const v3& maxi,
                                         const v3& color )
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

  void Debug3DDrawData::DebugDraw3DTriangle( const v3& p0,
                                             const v3& p1,
                                             const v3& p2,
                                             const v3& color0,
                                             const v3& color1,
                                             const v3& color2 )
  {
    DebugDraw3DLine( p0, p1, color0, color1 );
    DebugDraw3DLine( p0, p2, color0, color2 );
    DebugDraw3DLine( p1, p2, color1, color2 );
  }

  void Debug3DDrawData::DebugDraw3DTriangle( const v3& p0,
                                             const v3& p1,
                                             const v3& p2,
                                             const v3& color )
  {
    DebugDraw3DTriangle( p0, p1, p2, color, color, color );
  }

  void Debug3DDrawData::DebugDraw3DToTexture( Render::IContext* renderContext,
                                              const Render::TextureHandle,
                                              const Camera* camera,
                                              const int viewWidth,
                                              const int viewHeight,
                                              Errors& errors )
  {
#if TAC_TEMPORARILY_DISABLED()
    TAC_PROFILE_BLOCK;
    const int vertexCount = mDebugDrawVerts.size();
    if( mDebugDrawVerts.size() )
    {
      if( !mVerts.IsValid() || mCapacity < mDebugDrawVerts.size() )
      {
        if( mVerts.IsValid() )
          Render::DestroyVertexBuffer( mVerts, TAC_STACK_FRAME );
        mVerts = Render::CreateBuffer( mDebugDrawVerts.size() * sizeof( DefaultVertexColor ),
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

    const Render::DefaultCBufferPerFrame perFrameData = Debug3DGetPerFrameData( viewId,
                                                                          camera,
                                                                          viewWidth,
                                                                          viewHeight );

    const Render::BufferHandle hPerFrame = Render::DefaultCBufferPerFrame::Handle;
    const int size = sizeof( Render::DefaultCBufferPerFrame );
    Render::UpdateConstantBuffer( hPerFrame, &perFrameData, size, TAC_STACK_FRAME );

    Render::BeginGroup( "debug 3d", TAC_STACK_FRAME );
    Render::SetBlendState( gDebug3DCommonData.mAlphaBlendState );
    Render::SetDepthState( gDebug3DCommonData.mDepthLess );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::LineList );
    Render::SetShader( gDebug3DCommonData.m3DVertexColorShader );
    Render::SetRasterizerState( gDebug3DCommonData.mRasterizerStateNoCull );
    Render::SetVertexFormat( gDebug3DCommonData.mVertexColorFormat );
    Render::SetVertexBuffer( mVerts, 0, vertexCount );
    Render::SetIndexBuffer( Render::BufferHandle(), 0, 0 );
    Render::Submit( viewId, TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
#else
    mDebugDrawVerts.clear();
#endif
  }

}

