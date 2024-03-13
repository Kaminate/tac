#pragma once

#include "tac-engine-core/shell/tac_shell_timer.h"

namespace Tac
{

  struct ProfileFunction
  {
    void AppendChild( ProfileFunction* child );
  
    void DeepCopy( const ProfileFunction* );


    ProfileFunction* mParent = nullptr;
    ProfileFunction* mNext = nullptr;
    ProfileFunction* mChildren = nullptr;

    //               must be a time point, Timestep::GetElapsedTime()
    //               is constant for the entire frame
    Timepoint        mBeginTime;
    Timepoint        mEndTime;

    const char*      mName = nullptr;
  };
}

