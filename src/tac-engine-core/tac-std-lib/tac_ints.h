#pragma once

namespace Tac
{
  using i8 = signed char;
  using i16 = signed short;
  using i32 = signed int;
  using i64 = signed long long;

  using u8 = unsigned char;
  using u16 = unsigned short;
  using u32 = unsigned int;
  using u64 = unsigned long long;

  using UPtr = u64;
  using IPtr = i64;
  using size_t = UPtr;
  using SizeT = UPtr;
  using ptrdiff_t = IPtr;
  using PtrDiffT = IPtr;

  static_assert( sizeof( u8 ) == 1 );
  static_assert( sizeof( u16 ) == 2 );
  static_assert( sizeof( u32 ) == 4 );
  static_assert( sizeof( u64 ) == 8 );

  static_assert( sizeof( i8 ) == 1 );
  static_assert( sizeof( i16 ) == 2 );
  static_assert( sizeof( i32 ) == 4 );
  static_assert( sizeof( i64 ) == 8 );

  static_assert( sizeof( UPtr ) == sizeof( void* ) );
  static_assert( sizeof( IPtr ) == sizeof( void* ) );

  using r32 = float;
  using r64 = double;
  static_assert( sizeof( r32 ) == 4 );
  static_assert( sizeof( r64 ) == 8 );
}

