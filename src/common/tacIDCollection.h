#pragma once

namespace Tac
{
  struct IdCollection
  {
    void            Init( int capacity );
    void            Uninit();
    int             Alloc();
    void            Free( int id );
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
