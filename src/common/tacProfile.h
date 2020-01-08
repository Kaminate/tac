
#pragma once
#include "common/containers/tacVector.h"
#include "common/tacTime.h"

struct TacProfileBlock;
struct TacProfileFunction;
struct TacProfileSystem;

struct TacProfileBlock
{
  TacProfileBlock();
  ~TacProfileBlock();
  TacProfileFunction* mFunction;
  TacTimer mTimer;
};
#define TAC_PROFILE_BLOCK TacProfileBlock b##__LINE__ ();

struct TacProfileFunction
{
  TacProfileFunction();
  void Clear();

  TacProfileFunction* mNext;
  TacProfileFunction* mChildren;
  float mMiliseconds;
};

struct TacProfileSystem
{
  static TacProfileSystem* Instance;

  void                Init();
  void                OnFrameBegin();
  TacProfileFunction* Alloc();
  void                Dealloc(TacProfileFunction*);

  TacVector< TacProfileFunction* > mUsed;
  TacVector< TacProfileFunction* > mFree;
  TacVector< TacProfileFunction* > mProfileStack;
};

