#pragma once

// auto include assert macro to assert that invert exists
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT

namespace Tac
{
  struct v2; struct v3; struct v4;
  struct m3; struct m4;

  bool IsAboutZero( float );
  void AssertAboutZero( float );

  bool IsAboutEqual( float, float );
  void AssertAboutEqual( float, float );

  bool IsAboutEqual( const float*, const float*, int );
  void AssertAboutEqual( const float*, const float*, int );

  void AssertAboutEqual( const v2&, const v2& );
  void AssertAboutEqual( const v3&, const v3& );
  void AssertAboutEqual( const v4&, const v4& );
  void AssertAboutEqual( const m3&, const m3& );
  void AssertAboutEqual( const m4&, const m4& );
  
}

