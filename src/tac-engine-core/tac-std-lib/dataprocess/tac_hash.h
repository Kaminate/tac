#pragma once


#include "tac-std-lib/tac_ints.h"

//#include "tac-std-lib/tac_core.h"

namespace Tac { struct StringView; struct String; }// struct AssetPathString; }

namespace Tac
{
  using HashValue = u32;


  HashValue Hashu32( u32 );
  HashValue Hashu64( u64 );
  HashValue Hash( int );
  HashValue Hash( u64 );
  HashValue Hash( const char* );
  HashValue Hash( const char*, int );
  HashValue Hash( const StringView& );
  HashValue Hash( const String& );
  //HashValue Hash( const AssetPathString& ); // gross?

  template< typename T >
  HashValue Hash( T* t )
  {
    return Hash( ( UPtr )t );
  }

  template< typename T >
  HashValue Hash( T t )
  {
     //Cast if possible, ie from `int` or `enum`
    return Hashu32( ( HashValue )t );
    //return Hashu64( ( u64 )t );
  }

  HashValue Hash( HashValue*, int );
  HashValue Hash( HashValue );
  HashValue Hash( HashValue, HashValue );
  HashValue Hash( HashValue, HashValue, HashValue );
  HashValue Hash( HashValue, HashValue, HashValue, HashValue );
  HashValue Hash( HashValue, HashValue, HashValue, HashValue, HashValue );


  struct Hasher
  {
    void Eat( HashValue );
    operator HashValue() const;
    HashValue mHashValue {};
  };

} // namespace Tac
