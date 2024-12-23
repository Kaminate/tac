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

  static const int cylinderSegmentCount   { 10 };
  static const int hemisphereSegmentCount { 4 };
  static const int numdivisions           { 20 };

  struct Debug3DConstBuf
  {
    m4 mView;
    m4 mProj;
  };

  static struct Debug3DCommonData
  {
    void                          Init( Errors& );
    void                          Uninit();

    Render::BlendState            GetAlphaBlendState() const;
    Render::DepthState            GetDepthLess() const;
    Render::RasterizerState       GetRasterizerStateNoCull() const;
    Render::VertexDeclarations    GetVertexColorFormat() const;
    Render::ProgramParams         GetProgramParams() const;
    Render::CreateBufferParams    GetConstBufParams() const;
    Render::PipelineParams        GetPipelineParams() const;

    Render::ProgramHandle         m3DVertexColorShader;
    Render::BufferHandle          mConstBuf;
    Render::PipelineHandle        mPipeline;
    Render::IShaderVar*           mShaderConstBuf;
  } gDebug3DCommonData;

  static m4 Debug3DGetProj( const Camera* camera, const v2i viewSize )
  {
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const auto ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projMtxParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { ( float )viewSize.x / ( float )viewSize.y },
      .mFOVYRadians   { camera->mFovyrad },
    };

    return m4::ProjPerspective( projMtxParams );
  }
  

  void Debug3DCommonDataInit( Errors& errors )
  {
    gDebug3DCommonData.Init( errors );
  }

  void Debug3DCommonDataUninit()
  {
    gDebug3DCommonData.Uninit();
  }

  void Debug3DCommonData::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DVertexColorShader );
    renderDevice->DestroyPipeline( mPipeline );
  }

  Render::BlendState            Debug3DCommonData::GetAlphaBlendState() const
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::SrcA },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add},
    };
  }

  Render::DepthState            Debug3DCommonData::GetDepthLess() const
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  Render::RasterizerState       Debug3DCommonData::GetRasterizerStateNoCull() const
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           {}, 
    };
  }

  Render::VertexDeclarations    Debug3DCommonData::GetVertexColorFormat() const
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( DefaultVertexColor, mPosition ) },
    };

    const Render::VertexDeclaration colDecl
    {
      .mAttribute         { Render::Attribute::Color },
      .mFormat            { Render::VertexAttributeFormat::GetVector4() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( DefaultVertexColor, mColor )},
    };

    Render::VertexDeclarations decls;
    decls.push_back( posDecl );
    decls.push_back( colDecl );
    return decls;
  }

  Render::ProgramParams         Debug3DCommonData::GetProgramParams() const
  {
    return Render::ProgramParams
    {
      .mInputs     { "3DDebug" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  Render::CreateBufferParams    Debug3DCommonData::GetConstBufParams() const
  {
    return Render::CreateBufferParams 
    {
      .mByteCount     { sizeof( Debug3DConstBuf ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "3DDebug" },
    };
  }

  Render::PipelineParams        Debug3DCommonData::GetPipelineParams() const
  {
    const Render::RasterizerState rasterizerState{ GetRasterizerStateNoCull() };
    const Render::DepthState depthStateData{ GetDepthLess() };
    const Render::VertexDeclarations decls{ GetVertexColorFormat() };
    const Render::BlendState alphaBlendStateData{ GetAlphaBlendState() };
    return Render::PipelineParams
    {
      .mProgram           { m3DVertexColorShader },
      .mBlendState        { alphaBlendStateData },
      .mDepthState        { depthStateData},
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { decls},
      .mPrimitiveTopology { Render::PrimitiveTopology::LineList },
      .mName              { "debug-3d-pso"},
    };
  }

  void Debug3DCommonData::Init( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::ProgramParams programParams{ GetProgramParams() };
    TAC_CALL( m3DVertexColorShader = renderDevice->CreateProgram( programParams, errors ) );

    const Render::PipelineParams pipelineParams{ GetPipelineParams() };
    TAC_CALL( mPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    const Render::CreateBufferParams createConstBuf{ GetConstBufParams() };
    TAC_CALL( mConstBuf = renderDevice->CreateBuffer( createConstBuf, errors ) );

    mShaderConstBuf = renderDevice->GetShaderVariable( mPipeline, "constBuf" );
    mShaderConstBuf->SetResource( mConstBuf );
  }

  // -----------------------------------------------------------------------------------------------

  Debug3DDrawBuffers::Buffer::~Buffer()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyBuffer( mVtxBuf );
  }

  const Debug3DDrawBuffers::Buffer* Debug3DDrawBuffers::Update(
    Render::IContext* renderContext,
    Span< DefaultVertexColor > vtxs,
    Errors& errors )
  {
    const int frameCount{ Render::RenderApi::GetMaxGPUFrameCount() };
    mBuffers.resize( frameCount );
    Debug3DDrawBuffers::Buffer* buffer{ &mBuffers[ mRenderBufferIdx ] };
    ++mRenderBufferIdx %= frameCount;

    const int stride{ ( int )sizeof( DefaultVertexColor ) };
    const int requiredVtxCount { vtxs.size() };
    const int requiredByteCount{ requiredVtxCount * stride };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::BufferHandle& mRenderVtxBuf{ buffer->mVtxBuf };

    if( !mRenderVtxBuf.IsValid() || requiredByteCount > buffer->mVtxBufByteCapacity )
    {
      renderDevice->DestroyBuffer( mRenderVtxBuf );
      const Render::CreateBufferParams vtxBufCreate
      {
        .mByteCount     { requiredByteCount },
        .mBytes         {},
        .mStride        { stride },
        .mUsage         { Render::Usage::Dynamic },
        .mBinding       { Render::Binding::VertexBuffer },
        .mOptionalName  { "debug-3d-vtx" },
      };

      TAC_CALL_RET( mRenderVtxBuf = renderDevice->CreateBuffer( vtxBufCreate, errors ) );
      buffer->mVtxBufByteCapacity = requiredByteCount;
    }

    buffer->mVtxCount = requiredVtxCount;

    const Render::UpdateBufferParams vtxBufUpdate
    {
      .mSrcBytes     { vtxs.data() },
      .mSrcByteCount { requiredByteCount },
    };

    TAC_CALL_RET( renderContext->UpdateBuffer( mRenderVtxBuf, vtxBufUpdate, errors ) );
    
    return buffer;
  }

  // -----------------------------------------------------------------------------------------------


  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1 )
  {
    DebugDraw3DLine( p0, p1, v4( 1, 1, 1, 1 ) );
  }

  void Debug3DDrawData::DebugDraw3DLine( const v3& p0,
                                         const v3& p1,
                                         const v3& color0,
                                         const v3& color1 )
  {
    if constexpr( !kIsDebugMode )
      return;

    const DefaultVertexColor v0
    {
       .mPosition { p0 },
       .mColor    { v4( color0, 1.0f ) } ,
    };
    const DefaultVertexColor v1
    {
       .mPosition { p1 },
       .mColor    { v4( color1, 1.0f ) },
    };

    mDebugDrawVerts.push_back( v0 );
    mDebugDrawVerts.push_back( v1 );
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
    if constexpr( !kIsDebugMode )
      return;

    const DefaultVertexColor v0
    {
       .mPosition { p0 },
       .mColor    {color0} ,
    };
    const DefaultVertexColor v1
    {
       .mPosition { p1 },
       .mColor    { color1},
    };

    mDebugDrawVerts.push_back( v0 );
    mDebugDrawVerts.push_back( v1 );
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
    const float length { Length( dir ) };
    if( length < 0.001f || Abs( rad ) < 0.001f )
      return;

    const v3 unitDir { dir / length };
    v3 tan1;
    v3 tan2;
    GetFrameRH( unitDir, tan1, tan2 );
    tan1 *= rad;
    tan2 *= rad;
    v3 prevPoint { p0 + tan1 };
    for( int i { 1 }; i <= numdivisions; ++i )
    {
      const float theta { 3.14f * 2.0f * ( float )i / ( float )numdivisions };
      const v3 point{ p0 + Cos( theta ) * tan1 + Sin( theta ) * tan2 };
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
    v3 dir { p1 - p0 };
    const float dirlen { Length( dir ) };
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
    float hemisphereRads {  };
    float dHemisphereRads { ( 3.14f * 0.5f ) / hemisphereSegmentCount };
    v3 tan1;
    v3 tan2;
    GetFrameRH( mDirection, tan1, tan2 );
    for( int iCylinder {  }; iCylinder < cylinderSegmentCount; ++iCylinder )
      mPrevPts[ iCylinder ] = mOrigin + mDirection * radius;

    for( int iHemisphere {  }; iHemisphere < hemisphereSegmentCount; ++iHemisphere )
    {
      hemisphereRads += dHemisphereRads;
      float circleRadius { radius * Sin( hemisphereRads ) };
      float circleOffset { radius * Cos( hemisphereRads ) };
      v3 circleOrigin { mOrigin + mDirection * circleOffset };

      float circleRads {  };
      float dCircleRads { ( 3.14f * 2.0f ) / cylinderSegmentCount };
      for( int iCylinder {  }; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        mCurrPts[ iCylinder ]
          = circleOrigin
          + tan1 * circleRadius * Cos( circleRads )
          + tan2 * circleRadius * Sin( circleRads );
        circleRads += dCircleRads;
      }

      for( int iCylinder {}; iCylinder < cylinderSegmentCount; ++iCylinder )
      {
        const int iCylinderNext { ( iCylinder + 1 ) % cylinderSegmentCount };
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
    v3 dir { p1 - p0 };
    const float dirlen { Length( dir ) };
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
    for( int iSegment {}; iSegment < cylinderSegmentCount; ++iSegment )
    {
      const float angle { ( 3.14f * 2.0f ) * iSegment / ( float )cylinderSegmentCount };
      const float s { Sin( angle ) };
      const float c { Cos( angle ) };
      const v3 offset { c * tan1 + s * tan2 };
      p0Points[ iSegment ] = p0 + offset;
      p1Points[ iSegment ] = p1 + offset;
    }

    for( int iSegment {}; iSegment < cylinderSegmentCount; ++iSegment )
    {
      const int iSegmentNext { ( iSegment + 1 ) % cylinderSegmentCount };
      DebugDraw3DLine( p0Points[ iSegment ], p0Points[ iSegmentNext ], color );
      DebugDraw3DLine( p1Points[ iSegment ], p1Points[ iSegmentNext ], color );
      DebugDraw3DLine( p0Points[ iSegment ], p1Points[ iSegment ], color );
    }
  }

  void Debug3DDrawData::DebugDraw3DGrid( const v3& lineColor )
  {
    const float d { 10.0f }; // Fade distance
    const v4 f( lineColor, 0.0f ); // Far Color
    for( float i { -d }; i <= d; i += 1.0f )
    {
      const float a { 1 - ( Abs( i ) / d ) };
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
    const v3 arrowDiff { to - from };
    const float arrowDiffLenSq { Quadrance( arrowDiff ) };
    if( arrowDiffLenSq < 0.001f )
      return;

    const float arrowDiffLen { Sqrt( arrowDiffLenSq ) };
    const v3 arrowDir { arrowDiff / arrowDiffLen };

    const float arrowHeadLen { arrowDiffLen * 0.2f };
    const float arrowHeadRadius { arrowHeadLen * 0.4f };

    v3 tan1, tan2;
    GetFrameRH( arrowDir, tan1, tan2 );
    tan1 *= arrowHeadRadius;
    tan2 *= arrowHeadRadius;
    const v3 circlePos = to - arrowDir * arrowHeadLen;
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
    const int pointCount { 8 };
    v3 original_points[]  {
      v3( -1, -1, -1 ),
      v3( -1, -1, 1 ),
      v3( -1, 1, -1 ),
      v3( -1, 1, 1 ),
      v3( 1, -1, -1 ),
      v3( 1, -1, 1 ),
      v3( 1, 1, -1 ),
      v3( 1, 1, 1 ) };
    v3 transfor_points[ pointCount ];
    const m4 transform { m4::Transform( halfextents, orientation, pos ) };
    for( int i{}; i < pointCount; ++i )
      transfor_points[ i ] = ( transform * v4( original_points[ i ], 1.0f ) ).xyz();

    for( int i{}; i < pointCount; ++i )
    {
      const v3& original_point_i = original_points[ i ];
      const v3& transfor_point_i = transfor_points[ i ];
      for( int j = i + 1; j < pointCount; ++j )
      {
        const v3& original_point_j = original_points[ j ];
        const v3& transfor_point_j = transfor_points[ j ];

        int numSame {};
        for( int axis {}; axis < 3; ++axis )
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
    m3 orientation { m3::RotRadEuler(eulerAnglesRad) };
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

    for( int a0 {}; a0 < 3; ++a0 )
    {
      v3 v0;
      v3 v0Color;
      v0[ a0 ] = mini[ a0 ];
      v0Color[ a0 ] = miniColor[ a0 ];

      v3 v1;
      v3 v1Color;
      v1[ a0 ] = maxi[ a0 ];
      v1Color[ a0 ] = maxiColor[ a0 ];
      int a1 { ( a0 + 1 ) % 3 };
      int a2 { ( a0 + 2 ) % 3 };
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
    for( int a0 {}; a0 < 3; ++a0 )
    {
      v3 v0;
      v0[ a0 ] = mini[ a0 ];

      v3 v1;
      v1[ a0 ] = maxi[ a0 ];
      int a1 { ( a0 + 1 ) % 3 };
      int a2 { ( a0 + 2 ) % 3 };
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

  void Debug3DDrawData::CopyFrom( const Debug3DDrawData& other )
  {
    mDebugDrawVerts = other.mDebugDrawVerts;
  }

    void Debug3DDrawData::Clear()
    {
      mDebugDrawVerts.clear();
    }

    Span< DefaultVertexColor > Debug3DDrawData::GetVerts()
    {
      return { mDebugDrawVerts.data(), mDebugDrawVerts.size() };
    }
  

  void Debug3DDrawBuffers::Buffer::DebugDraw3DToTexture( Render::IContext* renderContext,
                                                         const Render::TextureHandle renderTargetColor,
                                                         const Render::TextureHandle renderTargetDepth,
                                                         const Camera* camera,
                                                         const v2i viewSize,
                                                         Errors& errors ) const
  {
    TAC_PROFILE_BLOCK;

    if( !mVtxCount )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Debug3DConstBuf constBufData
    {
      .mView { camera->View() },
      .mProj { Debug3DGetProj( camera, viewSize ) },
    };

    const Render::UpdateBufferParams updateConstBuf
    {
      .mSrcBytes     { &constBufData },
      .mSrcByteCount { sizeof( Debug3DConstBuf ) },
    };
    renderContext->UpdateBuffer(  gDebug3DCommonData.mConstBuf, updateConstBuf, errors );

    const Render::DrawArgs drawArgs
    {
      .mVertexCount { mVtxCount },
      .mStartVertex {},
    };

    const Render::Targets renderTargets
    {
      .mColors { renderTargetColor },
      .mDepth  { renderTargetDepth },
    };

    TAC_RENDER_GROUP_BLOCK( renderContext, "debug 3d" );
    renderContext->SetPipeline( gDebug3DCommonData.mPipeline );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::LineList );
    renderContext->SetVertexBuffer( mVtxBuf );
    renderContext->SetIndexBuffer( {} );
    renderContext->CommitShaderVariables();
    renderContext->Draw( drawArgs );
  }

} // namespace Tac

