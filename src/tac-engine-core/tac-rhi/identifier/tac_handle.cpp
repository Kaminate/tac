#include "tac_handle.h" // self-inc

namespace Tac
{
  // Operator unsigned and -1 are useful for comparing against if( handle < unsigned_max_foos ) and for hashing

  Handle::Handle( int index ) : mIndex( index )         {                                 }
  Handle::operator int() const                          { return mIndex;                  }
  Handle::operator unsigned() const                     { return mIndex;                  }
  bool Handle::operator ==( const Handle handle ) const { return mIndex == handle.mIndex; }
  bool Handle::operator !=( const Handle handle ) const { return mIndex != handle.mIndex; }
  bool Handle::IsValid() const                          { return mIndex != -1;            }
  int  Handle::GetIndex() const                         { return mIndex;                  }
}
