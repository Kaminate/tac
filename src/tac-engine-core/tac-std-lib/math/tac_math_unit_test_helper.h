#pragma once

// auto include assert macro to assert that invert exists
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT

namespace Tac
{
  struct v2; struct v3; struct v4;
  struct m2; struct m3; struct m4;

  inline const float kDiff = 0.001f;

  bool IsAboutZero( float, float = kDiff );
  void AssertAboutZero( float, float = kDiff );

  bool IsAboutEqual( float, float, float = kDiff );
  void AssertAboutEqual( float, float, float = kDiff );

  bool IsAboutEqual( const float*, const float*, int, float = kDiff );
  void AssertAboutEqual( const float*, const float*, int, float = kDiff );

  void AssertAboutEqual( const v2&, const v2&, float = kDiff );
  void AssertAboutEqual( const v3&, const v3&, float = kDiff );
  void AssertAboutEqual( const v4&, const v4&, float = kDiff );
  void AssertAboutEqual( const m2&, const m2&, float = kDiff );
  void AssertAboutEqual( const m3&, const m3&, float = kDiff );
  void AssertAboutEqual( const m4&, const m4&, float = kDiff );
  
}

