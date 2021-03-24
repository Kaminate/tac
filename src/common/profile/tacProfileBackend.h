#pragma once

#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"

#include <mutex>
#include <map>

namespace Tac
{

  struct ProfileFunction
  {
    ProfileFunction* mParent = nullptr;
    ProfileFunction* mNext = nullptr;
    ProfileFunction* mChildren = nullptr;
    double           mBeginTime;
    double           mEndTime;
    StackFrame       mStackFrame;
  };

  const float kProfileStoreSeconds = 0.1f;

  struct ProfileFunctionPool
  {
    ProfileFunction*           Alloc();
    void                       Dealloc(ProfileFunction*);
    Vector< ProfileFunction* > sFunctionsUnused;
  };

  typedef std::list< ProfileFunction* >                     ProfiledFunctionList;
  typedef std::map< std::thread::id, ProfiledFunctionList > ProfiledFunctions;

  ProfiledFunctions CopyProfiledFunctions(ProfileFunctionPool*);
}

