#pragma once

//#include "src/common/tacPreprocessor.h"

namespace Tac
{
  struct IdCollection
  {
    //IdCollection() = default;
    //IdCollection( int capacity ) { Init( capacity ); }
    IdCollection( int capacity );
    //void            Init( int capacity );
    void            Uninit();
    int             Alloc();
    void            Free( int id );
    //void            Free( const HandleBase& handleBase );
    int*            begin();
    int*            end();
  private:
    int*            GetDense();
    int*            GetSparse();

    int             mCapacity = 0;
    int*            mData = nullptr;
    int             mSize = 0;
  };

  void UnitTestIdCollection();
}
