#include "tac_jppt_Scene.h"

namespace Tac::gpupt
{

  m4  Instance::GetModelMatrix() const
  {
    return m4::Transform( mScale, ..., mPosition );
  }

  void Shape::CalculateTangents()
  {
    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping

    const int nVerts{ mPositions.size() };
    const int nTexCoords{ mTexCoords.size() };
    const int nTris{ mTriangles.size() };

    Vector< v3 > tan0( nVerts, {} );
    Vector< v3 > tan1( nVerts, {} );
    mTangents.resize( nVerts );
    if( nTexCoords != nVerts )
      return;

    for( u32 i{}; i < nTris; ++i )
    {
      v3i tri{ mTriangles[ i ] };
      int i0{ tri[ 0 ] };
      int i1{ tri[ 1 ] };
      int i2{ tri[ 2 ] };

      v3 v0{ mPositions[ i0 ] };
      v3 v1{ mPositions[ i1 ] };
      v3 v2{ mPositions[ i2 ] };

      v2 w0{ mTexCoords[ i0 ] };
      v2 w1{ mTexCoords[ i1 ] };
      v2 w2{ mTexCoords[ i2 ] };

      v3 e1{ v1 - v0 };
      v3 e2{ v2 - v0 };

      v2 st1{ w1 - w0 };
      v2 st2{ w2 - w0 };

      const matrix2 m
      {
        st1.x, st1.y,
        st2.x, st2.y
      };

      const float r{ 1 / m.determinant() };

      v3 sdir{ ... };
      v3 tdir{ ... };

      tan0[ i0 ] += sdir;
      tan0[ i1 ] += sdir;
      tan0[ i2 ] += sdir;

      tan1[ i0 ] += tdir;
      tan1[ i1 ] += tdir;
      tan1[ i2 ] += tdir;
    }

    for( u32 i{}; i < nVerts; ++i )
    {
      v3 n{ mNormals[i] };
      v3 t{ tan1[ i ] };
      mTangents[ i ] = v4( Normalize( t - n * Dot( n, t ) ), 1 );
      mTangents[ i ].w = Dot( Cross( n, t ), tan2[ i ] ) < 0 ? -1 : 1;
    }

  }

  Scene* Scene::CreateCornellBox()
  {
    return {};
  }
} // namespace Tac::gpupt
