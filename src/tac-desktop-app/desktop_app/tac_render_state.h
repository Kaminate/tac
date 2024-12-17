#if 0
#pragma once

#include "tac-std-lib/containers/tac_list.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h" // App

namespace Tac
{
  struct GameStateManager
  {
    struct Element
    {
      App::IState* mState       {};
      int          mUsedCounter {};
    };

    struct Pair
    {
      ~Pair();
      bool IsValid() const { return mOldState && mNewState; }

      App::IState* mOldState       {};
      int*         mOldUsedCounter {};

      App::IState* mNewState       {};
      int*         mNewUsedCounter {};
    };


    void Enqueue( App::IState* ); // takes ownership
    Pair Dequeue();

  private:
    void CleanUnneeded();

    List< Element > mElements;
  };
} // namespace Tac
#endif
