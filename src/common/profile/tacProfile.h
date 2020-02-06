
#pragma once
#include "common/containers/tacVector.h"
#include "common/tacTime.h"
#include "common/tacPreprocessor.h"

struct TacProfileBlock;
struct TacProfileFunction;
struct TacProfileSystem;

struct TacProfileBlock
{
  TacProfileBlock(TacStackFrame stackFrame);
  ~TacProfileBlock();
  TacProfileFunction* mFunction = nullptr;
};
#define TAC_PROFILE_BLOCK TacProfileBlock b##__LINE__ (TAC_STACK_FRAME);

struct TacProfileFunction
{
  void                Clear();
  TacProfileFunction* GetLastChild();
  void                AppendChild( TacProfileFunction* child );
  
  TacProfileFunction* mNext = nullptr;
  TacProfileFunction* mChildren = nullptr;
  TacTimepoint        mBeginTime;
  TacTimepoint        mEndTime;
  TacStackFrame       mStackFrame;
};

struct TacProfileSystem
{
  static TacProfileSystem* Instance;
  void                     Init();
  void                     OnFrameBegin();
  void                     OnFrameEnd();
  TacProfileFunction*      Alloc();
  void                     Dealloc( TacProfileFunction* );
  void                     PushFunction( TacProfileFunction* );

  TacVector< TacProfileFunction* > mFree;
  TacProfileFunction*              mLastFrame = nullptr;
  TacProfileFunction*              mCurrFrame = nullptr;
  TacVector< TacProfileFunction* > mCurrStackFrame;
};

