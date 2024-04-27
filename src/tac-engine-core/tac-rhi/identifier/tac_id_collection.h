#pragma once

#include "tac-std-lib/containers/tac_vector.h"

namespace Tac
{

  struct IdCollection
  {
    ~IdCollection();

    int    Alloc();
    void   Free( int );
    int    size() const          { return mSize; }

    // Iterate through allocated ids
    int*       begin()           { return mDense; }
    const int* begin() const     { return mDense; }
    int*       end()             { return mDense + mSize; }
    const int* end() const       { return mDense + mSize; }

  private:
    void   reserve( int );

    int    mCapacity {}; // Capacity of both the dense and sparse arrays
    int    mSize     {}; // Number of allocated IDs
    int*   mDense    {};
    int*   mSparse   {};
  };

  void UnitTestIdCollection();

} // namespace Tac

