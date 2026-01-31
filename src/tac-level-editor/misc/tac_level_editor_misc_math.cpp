#include "tac_level_editor_misc_math.h" // self-inc

#include "tac-level-editor/tac_level_editor.h"
#include "tac-engine-core/thirdparty/GTE/Mathematics/SymmetricEigensolver3x3.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-ecs/entity/tac_entity.h"

namespace Tac
{

  void MiscMath::DrawPlaneOfBestFit()
  {
    // TEMP BEGIN ( plane of best fit )
    // based off
    //   https://nvlpubs.nist.gov/nistpubs/jres/103/6/j36sha.pdf
    //   Least-Squares Fitting Algorithms of the NIST Algorithm Testing System
    {
      World* world{ Creation::GetWorld() };
      if( const int n{ world->mEntities.size()}; n )
      {
        gte::SymmetricEigensolver3x3<float> solver;
        std::array< float, 3 > eigenValues;
        std::array< std::array< float, 3 >, 3 > eigenVectors;

        const float oneOverN{ 1.0f / n };
        v3 avgPosition{};
        for( Entity* entity : world->mEntities )
          avgPosition += entity->mWorldPosition * oneOverN;

        v3 minPos{ avgPosition };
        for( Entity* entity : world->mEntities )
          minPos = Min( minPos, entity->mWorldPosition );

        v3 maxPos{ avgPosition };
        for( Entity* entity : world->mEntities )
          maxPos = Max( maxPos, entity->mWorldPosition );

        Vector< v3 > M;
        M.reserve( n );
        for( Entity* entity : world->mEntities )
          M.push_back( entity->mWorldPosition - avgPosition );

        m3 a{};
        for( int r{}; r < 3; ++r )
          for( int c{ r }; c < 3; ++c )
            for( int i{}; i < n; ++i )
              a( r, c ) += M[ i ][ r ] * M[ i ][ c ];

        solver( a.m00, a.m01, a.m02, a.m11, a.m12, a.m22, false, 1 , eigenValues, eigenVectors ) ;

        TAC_ASSERT( Abs( eigenValues[ 0 ] ) < Abs( eigenValues[ 1 ] ) );
        TAC_ASSERT( Abs( eigenValues[ 0 ] ) < Abs( eigenValues[ 2 ] ) );

        v3 circleOrigin{ avgPosition };
        v3 circleDir{ v3( eigenVectors[ 0 ][ 0 ],
                          eigenVectors[ 0 ][ 1 ],
                          eigenVectors[ 0 ][ 2 ] ) };
        float circleRad{ Distance( minPos, maxPos ) / 2 };
        world->mDebug3DDrawData->DebugDraw3DCircle( circleOrigin, circleDir, circleRad, v3( 1,1,0) );
      }
    }
  }


} // namespace Tac

