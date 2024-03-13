#include "tac_assert.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac
{
  //static AssertFns::Callback sCallback;

  //void AssertFns::SetCallback( Callback callback ) { sCallback = callback; }

  void AssertFns::HandleAssert( StringView message, StackFrame sf )
  {
    //if( sCallback )
    //  sCallback( message, sf );
    //else
    //  *( int* )( nullptr ) = 0xd34db33f;

    OS::OSDebugAssert( message, sf );
  }

  void AssertFns::HandleAssertCase( const char* name, int val, StackFrame sf )
  {
    const String str = String() + "Invalid switch case: " + name + " = " + Tac::ToString( val );
    HandleAssert( str, sf );
  }

  void AssertFns::HandleAssertIndex( unsigned i, unsigned n, StackFrame sf )
  {
    const String str = String()
      + "Index "
      + Tac::ToString( i )
      + " out of bounds "
      + Tac::ToString( n );
    HandleAssert( str, sf );
  }
}

