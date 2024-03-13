#include "tac_map.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/os/tac_os.h"

void Tac::MapUnitTest()
{
  Map< int, String > myMap;

  myMap[0] = "zero";
  myMap[1] = "one";
  myMap[2] = "two";

  {
    Map< int, String > mapA;
    mapA.insert_or_assign( 1, "hi" );

    // Check for double delete on copy constructor
    {
      Map< int, String > mapB( mapA );
    }

    // Check for double delete on operator =
    {
      Map< int, String > mapC;
      mapC = mapA;
    }
  }

  for( auto [first, second] : myMap )
  {
    const String str = String() + ToString( first ) + " " + second;
    OS::OSDebugPrintLine(str); 
  }

  if( auto it = myMap.Find( 0 ); it )
  {
    const String s = it.GetValue();
    OS::OSDebugPrintLine( s );
  }

  if( auto it = myMap.Find( 3 ); it != myMap.end() )
  {
    String s = it.GetValue();
    OS::OSDebugPrintLine( s );
  }

}

