#pragma once

#include "tac-std-lib/containers/tac_list.h"
//#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/tac_iapp.h"

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

  private:
    void CleanUnneeded();

    List< Element > mElements;
  };
} // namespace Tac
