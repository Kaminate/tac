#pragma once

#include "tac-engine-core/shell/tac_shell_time.h"

namespace Tac
{

  struct ProfileFunction
  {
    void AppendChild( ProfileFunction* child );
    void DeepCopy( const ProfileFunction* );

    ProfileFunction* mParent    {};
    ProfileFunction* mNext      {};
    ProfileFunction* mChildren  {};
    RealTime         mBeginTime {};
    RealTime         mEndTime   {};
    const char*      mName      {};
  };
}

