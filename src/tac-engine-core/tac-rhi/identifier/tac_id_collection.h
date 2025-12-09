#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{

  struct IdCollection
  {
    ~IdCollection();

    auto Alloc() -> int;
    void Free( int );
    auto size() const -> int            { return mSize;  }

    // Iterate through allocated ids
    auto begin() dynmc -> dynmc int*    { return mDense; }
    auto begin() const -> const int*    { return mDense; }
    auto end() dynmc -> dynmc int*      { return mDense + mSize; }
    auto end() const -> const int*      { return mDense + mSize; }

  private:
    void reserve( int );

    int    mCapacity {}; // Capacity of both the dense and sparse arrays
    int    mSize     {}; // Number of allocated IDs
    int*   mDense    {};
    int*   mSparse   {};
  };

  void UnitTestIdCollection();

} // namespace Tac

