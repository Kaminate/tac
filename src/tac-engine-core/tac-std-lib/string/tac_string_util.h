// Some helper functions
// ( non-templated, those would go in preprocessor )

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  template< typename T > String Join( const T& container, const StringView& seperator )
  {
    String result;
    for( const auto& element : container )
    {
      result += result.empty() ? StringView{} : seperator;
      result += element;
    }
    return result;
  }


  String ToLower( const StringView& );
  char   ToLower( char );
  bool   IsLower( char );
  char   ToUpper( char );
  bool   IsUpper( char );
  String FormatPercentage( float number_between_0_and_1 );
  String FormatPercentage( float curr, float maxi );
  bool   IsAscii( const StringView& );

  // input: "hello\nworld"
  // output: "+-------+\n"
  //         "| hello |\n"
  //         "| world |\n"
  //         "+-------+"
  String AsciiBoxAround( const StringView& );

}
