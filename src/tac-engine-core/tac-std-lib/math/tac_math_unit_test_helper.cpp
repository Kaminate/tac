#include "tac_math_unit_test_helper.h" // self-inc

#include "tac-std-lib/math/tac_math.h" // Abs
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_matrix4.h"

namespace Tac
{
  bool IsAboutZero( float f )
  {
    f = Abs( f );
    return f < 0.001f;
  }

  void AssertAboutZero( float f )
  {
    TAC_ASSERT( IsAboutZero( f ) );
  }

  bool IsAboutEqual( float a, float b )
  {
    return IsAboutZero( a - b );
  }

  void AssertAboutEqual( float a, float b )
  {
    TAC_ASSERT( IsAboutEqual( a, b ) );
  }

  bool IsAboutEqual( const float* as, const float* bs, int n )
  {
    for( int i{}; i < n; ++i )
      if( !IsAboutEqual( as[ i ], bs[ i ] ) )
        return false;
    return true;
  }

  void AssertAboutEqual( const float* as, const float* bs, int n )
  {
    TAC_ASSERT( IsAboutEqual( as, bs, n ) );
  }

  void AssertAboutEqual( const v2& a, const v2& b ) { AssertAboutEqual( a.data(), b.data(), 2 ); }
  void AssertAboutEqual( const v3& a, const v3& b ) { AssertAboutEqual( a.data(), b.data(), 3 ); }
  void AssertAboutEqual( const v4& a, const v4& b ) { AssertAboutEqual( a.data(), b.data(), 4 ); }
  void AssertAboutEqual( const m3& a, const m3& b ) { AssertAboutEqual( a.data(), b.data(), 3 * 3 ); }
  void AssertAboutEqual( const m4& a, const m4& b ) { AssertAboutEqual( a.data(), b.data(), 4 * 4 ); }
}

