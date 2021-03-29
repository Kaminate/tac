#pragma once

#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"

#include <mutex>
#include <map>
#include <list>

namespace Tac
{
  using ProfileClock = std::chrono::high_resolution_clock;
  using ProfileTimepoint = std::chrono::time_point< ProfileClock, std::chrono::nanoseconds >;

  struct ProfileFunction
  {
    ProfileFunction* mParent = nullptr;
    ProfileFunction* mNext = nullptr;
    ProfileFunction* mChildren = nullptr;

    //               must be a time point, ShellGetElapsedSeconds() is constant for the entire frame
    ProfileTimepoint mBeginTime;
    ProfileTimepoint mEndTime;
    const char*      mName;
  };

  const float kProfileStoreSeconds = 0.1f;

  struct ProfileFunctionVisitor
  {
    void             Visit( ProfileFunction* );
    virtual void     operator()( ProfileFunction* ) = 0;
  };

  typedef std::list< ProfileFunction* >                     ProfiledFunctionList;
  typedef std::map< std::thread::id, ProfiledFunctionList > ProfiledFunctions;

  ProfileTimepoint  ProfileTimepointGet();
  ProfileTimepoint  ProfileTimepointGetLastGameFrameBegin();
  float             ProfileTimepointSubtract( ProfileTimepoint, ProfileTimepoint );
  ProfileTimepoint  ProfileTimepointAddSeconds( ProfileTimepoint, float );
  ProfiledFunctions ProfiledFunctionCopy();
  void              ProfiledFunctionFree( ProfiledFunctions& );
}

