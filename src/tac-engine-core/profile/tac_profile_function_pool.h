#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/profile/tac_profile_function.h"

namespace Tac
{
  struct ProfileFunctionPool
  {
    ProfileFunction*           Alloc();
    ProfileFunction*           AllocCopy( const ProfileFunction* );
    void                       Dealloc( ProfileFunction* );

    Vector< ProfileFunction* > mFunctionsUnused;

    static thread_local ProfileFunctionPool sFunctionPool;
  };
}

