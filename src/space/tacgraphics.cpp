#include "common/tacPreprocessor.h"
#include "common/tacFont.h"
#include "common/math/tacMath.h"
#include "space/tacgraphics.h"
#include "space/taccomponent.h"

//#include "tacsay.h"
//#include "tacmodel.h"

//#include "common/imgui.h"
#include <cmath>

static const int cylinderSegmentCount = 10;
static const int hemisphereSegmentCount = 4;
static const int numdivisions = 20;
static const TacVector< TacComponentType > managedComponentTypes = {
  TacComponentType::Model,
};

TacDebugDrawAABB TacDebugDrawAABB::FromMinMax( v3 mini, v3 maxi )
{
  TacDebugDrawAABB result;
  result.mMini = mini;
  result.mMaxi = maxi;
  return result;
}
TacDebugDrawAABB TacDebugDrawAABB::FromPosExtents( v3 pos, v3 extents )
{
  TacDebugDrawAABB result;
  result.mMini = pos - extents;
  result.mMaxi = pos + extents;
  return result;
}

TacComponent* TacGraphics::CreateComponent( TacComponentType componentType )
{
  switch( componentType )
  {
    //case TacComponentType::Say:
    //{
    //  auto say = new TacSay();
    //  mSays.insert( say );
    //  return say;
    //}

  case TacComponentType::Model:
  {
    auto model = new TacModel();
    mModels.insert( model );
    return model;
  }
  }
  TacInvalidCodePath;
  return nullptr;
}

void TacGraphics::DestroyComponent( TacComponent* component )
{
  auto componentType = component->GetComponentType();
  switch( componentType )
  {
    //  case TacComponentType::Say:
    //  {
    //    auto say = ( TacSay* )component;
    //    auto it = mSays.find( say );
    //    TacAssert( it != mSays.end() );
    //    mSays.erase( it );
    //    delete say;
    //  } return;

  case TacComponentType::Model:
  {
    auto model = ( TacModel* )component;
    auto it = mModels.find( model );
    TacAssert( it != mModels.end() );
    mModels.erase( it );
    delete model;
  }return;
  }
  TacInvalidCodePath;
}

const TacVector< TacComponentType >& TacGraphics::GetManagedComponentTypes()
{
  return managedComponentTypes;
}
void TacGraphics::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Graphics" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );
}


void TacGraphics::DebugDrawLine( v3 p0, v3 p1, v3 color0, v3 color1 )
{
  if( !TacIsDebugMode() )
    return;
  TacDefaultVertexColor defVert0;
  defVert0.mColor = color0;
  defVert0.mPosition = p0;

  TacDefaultVertexColor defVert1;
  defVert1.mColor = color1;
  defVert1.mPosition = p1;

  mDebugDrawVerts.push_back( defVert0 );
  mDebugDrawVerts.push_back( defVert1 );
}
void TacGraphics::DebugDrawLine( v3 p0, v3 p1, v3 color )
{
  DebugDrawLine( p0, p1, color, color );
}
void TacGraphics::DebugDrawCircle( v3 p0, v3 dir, float rad, v3 color )
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
void TacGraphics::DebugDrawSphere( v3 origin, float radius, v3 color )
{
  DebugDrawCircle( origin, v3( 1, 0, 0 ), radius, color );
  DebugDrawCircle( origin, v3( 0, 1, 0 ), radius, color );
  DebugDrawCircle( origin, v3( 0, 0, 1 ), radius, color );
}
void TacGraphics::DebugDrawCapsule( v3 p0, v3 p1, float radius, v3 color )
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
void TacGraphics::DebugDrawHemisphere( v3 mOrigin, v3 mDirection, float radius, v3 color )
{
  TacVector< v3 > mPrevPts( cylinderSegmentCount );
  TacVector< v3 > mCurrPts( cylinderSegmentCount );
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
void TacGraphics::DebugDrawCylinder( v3 p0, v3 p1, float radius, v3 color )
{
  auto dir = p1 - p0;
  auto dirlen = Length( dir );
  if( dirlen < 0.001f )
    return;
  dir /= dirlen;
  TacVector< v3 > p0Points( cylinderSegmentCount );
  TacVector< v3 > p1Points( cylinderSegmentCount );
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
void TacGraphics::DebugDrawGrid( v3 lineColor )
{
  const int extent = 8;
  for( int i = -extent; i <= extent; ++i )
  {
    if( i == 0 )
    {
      for( int axis = 0; axis < 3; ++axis )
      {
        v3 to;
        to[ axis ] = ( float )extent;
        if( axis != 1 )
          DebugDrawLine( v3(), -to, lineColor );
        v3 arrowColor;
        arrowColor[ axis ] = 1;
        DebugDrawArrow( v3(), to, arrowColor );
      }
      continue;
    }
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
void TacGraphics::DebugDrawArrow( v3 from, v3 to, v3 color )
{
  auto arrowDiff = to - from;
  auto arrowDiffLenSq = TacQuadrance( arrowDiff );
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
void TacGraphics::DebugDrawOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color )
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
  auto transform = M4Transform( extents, eulerAnglesRad, pos );
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
void TacGraphics::DebugDrawAABB( TacDebugDrawAABB debugDrawAABB, v3 color )
{
  auto mini = debugDrawAABB.mMini;
  auto maxi = debugDrawAABB.mMaxi;
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
void TacGraphics::DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 )
{
  DebugDrawLine( p0, p1, color0, color1 );
  DebugDrawLine( p0, p2, color0, color2 );
  DebugDrawLine( p1, p2, color1, color2 );
}
void TacGraphics::DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color )
{
  DebugDrawTriangle( p0, p1, p2, color, color, color );
}
