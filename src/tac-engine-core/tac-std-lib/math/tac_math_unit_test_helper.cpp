#include "tac_math_unit_test_helper.h" // self-inc

#include "tac-std-lib/math/tac_math.h" // Abs
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix2.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_matrix4.h"

bool Tac::IsAboutZero( float f, float eps )
{
  f = Abs( f );
  return f < eps;
}

void Tac::AssertAboutZero( float f, float eps )
{
  TAC_ASSERT( IsAboutZero( f, eps ) );
}

bool Tac::IsAboutEqual( float a, float b, float eps )
{
  return IsAboutZero( a - b, eps );
}

void Tac::AssertAboutEqual( float a, float b, float eps )
{
  TAC_ASSERT( IsAboutEqual( a, b, eps ) );
}

bool Tac::IsAboutEqual( const float* as, const float* bs, int n, float eps )
{
  for( int i{}; i < n; ++i )
    if( !IsAboutEqual( as[ i ], bs[ i ], eps ) )
      return false;
  return true;
}

void Tac::AssertAboutEqual( const float* as, const float* bs, int n, float eps )
{
  TAC_ASSERT( IsAboutEqual( as, bs, n, eps ) );
}

void Tac::AssertAboutEqual( const v2& a, const v2& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 2, eps ); }
void Tac::AssertAboutEqual( const v3& a, const v3& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 3, eps ); }
void Tac::AssertAboutEqual( const v4& a, const v4& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 4, eps ); }
void Tac::AssertAboutEqual( const m2& a, const m2& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 2 * 2, eps ); }
void Tac::AssertAboutEqual( const m3& a, const m3& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 3 * 3, eps ); }
void Tac::AssertAboutEqual( const m4& a, const m4& b, float eps ) { AssertAboutEqual( a.data(), b.data(), 4 * 4, eps ); }

