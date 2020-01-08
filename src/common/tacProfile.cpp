#include "tacProfile.h"
#include "tacPreprocessor.h"

/////////////////////
// TacProfileBlock //
/////////////////////

TacProfileBlock::TacProfileBlock()
{
  TacProfileSystem* system = TacProfileSystem::Instance;
  TacProfileFunction* function = system->Alloc();

  mFunction = function;
  mTimer.Start();
}
TacProfileBlock::~TacProfileBlock()
{
  mTimer.Tick();

  float miliseconds = mTimer.mAccumulatedSeconds * 1000.0f;
  mFunction->mMiliseconds = miliseconds;

  TacProfileSystem* system = TacProfileSystem::Instance;
  TacAssert( system->mProfileStack.back() == mFunction );
}

////////////////////////
// TacProfileFunction //
////////////////////////

TacProfileFunction::TacProfileFunction()
{
  Clear();
}

void TacProfileFunction::Clear()
{
  mNext = nullptr;
  mChildren = nullptr;
  mMiliseconds = 0;
}

//////////////////////
// TacProfileSystem //
//////////////////////

TacProfileSystem* TacProfileSystem::Instance = nullptr;

void TacProfileSystem::Init()
{
}


TacProfileFunction* TacProfileSystem::Alloc()
{
  TacProfileFunction* result;
  if( mFree.size() )
  {
    result = mFree.back();
    mFree.pop_back();
  }
  else
  {
    result = new TacProfileFunction;
  }


  if( mProfileStack.empty() )
  {
    mProfileStack.push_back( result );
  }
  else
  {
    TacProfileFunction* backFunction = mProfileStack.back();
    if( backFunction->mChildren )
    {
      TacProfileFunction* lastChild = backFunction->mChildren;
      while( lastChild->mNext )
        lastChild = lastChild->mNext;
      lastChild->mNext = result;
    }
    else
    {
      backFunction->mChildren = result;
    }
  }

  mUsed.push_back( result );
  return result;
}

void TacProfileSystem::OnFrameBegin()
{
  for( TacProfileFunction* f : mUsed )
    mFree.push_back( f );
  mUsed.clear();
}

void TacProfileSystem::Dealloc(TacProfileFunction* fn)
{
  if( !fn )
    return;
  Dealloc( fn->mChildren );
  Dealloc( fn->mNext );
  fn->Clear();
  mFree.push_back( fn );
}

