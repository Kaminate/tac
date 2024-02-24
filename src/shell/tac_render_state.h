#pragma once

#include "src/common/containers/tac_list.h"
#include "src/shell/tac_desktop_app.h"

namespace Tac
{
  struct GameStateManager
  {
    struct Element
    {
      App::IState* mState;
      int mUsedCounter = 0;
    };

    struct Pair
    {
      ~Pair();
      bool IsValid() const { return mOldState && mNewState; }

      App::IState* mOldState;
      int*         mOldUsedCounter;

      App::IState* mNewState;
      int*         mNewUsedCounter;
    };


    void Enqueue( App::IState* ); // takes ownership
    Pair Dequeue();

    List< Element > mElements;
  };
} // namespace Tac
