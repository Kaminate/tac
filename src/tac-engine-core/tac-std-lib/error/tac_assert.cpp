#include "tac_assert.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{
  Vector< AssertFns::Callback > sCallbacks;

  void AssertFns::AddAssertCallback( Callback cb )
  {
    sCallbacks.push_back( cb );
  }

  void AssertFns::HandleAssert( StringView message, StackFrame sf )
  {
    const String str{ String()
      + "ASSERT FAILED( " + message + " ) "
      + "in " + sf.Stringify() };
    LogApi::LogMessagePrintLine( str, LogApi::kError );

    for( Callback cb : sCallbacks )
      cb( str );

    if constexpr( !kIsDebugMode )
    {
      std::exit( -1 );
    }
    // TODO: c++26 <debugging> std::is_debugger_present
  }

  void AssertFns::HandleAssertCase( const char* name, int val, StackFrame sf )
  {
    const String str { String() + "Invalid switch case: " + name + " = " + Tac::ToString( val ) };
    HandleAssert( str, sf );
  }

  void AssertFns::HandleAssertIndex( unsigned i, unsigned n, StackFrame sf )
  {
    const String str{ String()
      + "Index "
      + Tac::ToString( i )
      + " out of bounds "
      + Tac::ToString( n ) };
    HandleAssert( str, sf );
  }
}

