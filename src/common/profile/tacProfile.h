

#pragma once
#include "src/common/containers/tacVector.h"
#include "src/common/tacTime.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  struct ProfileBlock;
  struct ProfileFunction;
  struct ProfileSystem;

  struct ProfileBlock
  {
    ProfileBlock( Frame frame );
    ~ProfileBlock();
    ProfileFunction* mFunction = nullptr;
  };
#define TAC_PROFILE_BLOCK ProfileBlock b##__LINE__ (TAC_FRAME);

  struct ProfileFunction
  {
    void             Clear();
    ProfileFunction* GetLastChild();
    void             AppendChild( ProfileFunction* child );

    ProfileFunction* mNext = nullptr;
    ProfileFunction* mChildren = nullptr;
    Timepoint        mBeginTime;
    Timepoint        mEndTime;
    Frame            mFrame;
  };

  struct ProfileSystem
  {
    static thread_local ProfileSystem* Instance;
    ProfileSystem();
    void                       Init();
    void                       OnFrameBegin();
    void                       OnFrameEnd();
    ProfileFunction*           Alloc();
    void                       Dealloc( ProfileFunction* );
    void                       PushFunction( ProfileFunction* );

    Vector< ProfileFunction* > mFree;
    ProfileFunction*           mLastFrame = nullptr;
    ProfileFunction*           mCurrFrame = nullptr;
    Vector< ProfileFunction* > mCurrStack;
  };


}

