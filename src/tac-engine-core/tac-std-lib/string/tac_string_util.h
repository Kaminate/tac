// Some helper functions
// ( non-templated, those would go in preprocessor )

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  template< typename T >
  auto Join( const T& container, const StringView seperator ) -> String
  {
    String result;
    for( const auto& element : container )
    {
      result += result.empty() ? StringView{} : seperator;
      result += element;
    }
    return result;
  }

  auto ToLower( StringView ) -> String;
  char ToLower( char );
  bool IsLower( char );
  char ToUpper( char );
  bool IsUpper( char );
  auto FormatPercentage( float number_between_0_and_1 ) -> String;
  auto FormatPercentage( float curr, float maxi ) -> String;
  bool IsAscii( StringView );


}
