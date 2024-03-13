#include "tac_forward_list.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

void Tac::ForwardListUnitTest()
{
  ForwardList< int > myList;
  myList.PushFront( 3 );
  myList.PushFront( 2 );
  myList.PushFront( 1 );


  ForwardList< String > myStrs;
  myStrs.PushFront( "hello" );
  myStrs.PushFront( "world" );

  // test double delete on operator =
  {
    ForwardList< String > myStrs2;
    myStrs2 = myStrs;
  }

  // test double delete on copy ctor
  {
    ForwardList< String > myStrs3(myStrs);
  }

  for( const int i : myList )
  {
    OS::OSDebugPrintLine( ToString( i ) );
  }

  while( !myList.empty() )
  {
    const int val = myList.PopFront();
    OS::OSDebugPrintLine( ToString( val ) );
  }
}
