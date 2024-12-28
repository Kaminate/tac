#pragma once

namespace Tac
{
  template< typename T > struct IsPointer       { static constexpr bool value{}; };
  template< typename T > struct IsPointer< T* > { static constexpr bool value{ true }; };
  template< typename T > constexpr bool kIsPointer = IsPointer< T >::value;
}

