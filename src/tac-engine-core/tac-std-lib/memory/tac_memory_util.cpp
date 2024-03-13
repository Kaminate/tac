#include "tac-std-lib/memory/tac_memory_util.h" // self-include

//#include "tac-std-lib/memory/tac_frame_memory.h"
//#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{

  String FormatBytes( const int byteCount ) { return FormatBytes( { .mByteCount = byteCount } ); }

  String FormatBytes(const FormatByteSpec& spec )
  {
    int byteCount = spec.mByteCount;
    TAC_ASSERT( byteCount != -1 );

    if( !byteCount )
      return "0 Bytes";

    //char buf[ 100 ];
    //int n = 0;

    String result;

    const char* sizeStrs[] = { "Bytes", "KB", "MB", "GB", "TB" };

    int denomination = 1;
    int i = 0;
    bool approx = false;
    bool carry = false;
    while( byteCount )
    {
      int n = byteCount % 1024;
      if( n )
      {
        if( denomination >= spec.mMinDenomination )
        {
          TAC_ASSERT_INDEX( i, TAC_ARRAY_SIZE( sizeStrs ) );
          result = ToString( n ) + sizeStrs[ i ] + ( result.empty() ? String() : " " + result );
        }
        else
        {
          approx = true;
          carry = true;
        }
      }

      i++;
      byteCount /= 1024;
      denomination *= 1024;

      if( carry )
      {
        carry = false;
        byteCount++;
      }
    }

    if( approx )
      result = "<" + result;

    return result;

  }
}

