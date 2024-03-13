#include "tac_set.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_view.h"

void Tac::SetUnitTest()
{
  Set<int> mySet;
  mySet.insert( 1 );
  mySet.insert( 2 );
  mySet.insert( 3 );
  TAC_ASSERT( false == mySet.insert( 3 ) );
  TAC_ASSERT( true == mySet.Contains( 2 ) );
  TAC_ASSERT( false == mySet.Contains( 4 ) );

  for( int i : mySet )
  {
    OS::OSDebugPrintLine( ToString( i ) );
  }
}
