#include "tac_list.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_view.h"

void Tac::ListUnitTest()
{
  List< String > myStrs;
  myStrs.push_back( "hello" );
  {
    List< String > myStrsCopyCtor( myStrs );
  }
  {
    List< String > myStrsAssignOp;
    myStrsAssignOp = myStrs;
  }

  List< int > myList;
  myList.push_front( 3 );
  myList.push_front( 2 );
  myList.push_front( 1 );
  myList.push_back( 100 );
  myList.push_back( 4 );
  myList.push_back( 5 );
  myList.push_back( 6 );



  if( auto it = myList.Find( 100 ) )
    myList.erase( it );

  // should print 123456
  for( const int i : myList )
  {
    OS::OSDebugPrintLine( ToString( i ) );
  }


  while( !myList.empty() )
  {
    const int val = myList.pop_front();
    OS::OSDebugPrintLine( ToString( val ) );
  }
}
