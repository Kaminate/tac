#include "tacProfile.h"
#include "tacPreprocessor.h"

////////////////////////
// TacProfileFunction //
////////////////////////

void TacProfileFunction::Clear()
{
  TacAssert( !mChildren );
  TacAssert( !mNext );
  *this = TacProfileFunction();
}

TacProfileFunction* TacProfileFunction::GetLastChild()
{
  if( !mChildren )
    return nullptr;
  TacProfileFunction* child = mChildren;
  while( child->mNext )
    child = child->mNext;
  return child;
}

void TacProfileFunction::AppendChild( TacProfileFunction* child )
{
  TacProfileFunction* lastChild = GetLastChild();
  if( lastChild )
    lastChild->mNext = child;
  else
    mChildren = child;
}

/////////////////////
// TacProfileBlock //
/////////////////////

TacProfileBlock::TacProfileBlock( TacStackFrame stackFrame )
{
  TacProfileSystem* system = TacProfileSystem::Instance;
  mFunction = system->Alloc();
  mFunction->mStackFrame = stackFrame;
  system->PushFunction( mFunction );
  mTimer.Start();
}

TacProfileBlock::~TacProfileBlock()
{
  mTimer.Tick();
  mFunction->mMiliseconds = mTimer.mAccumulatedSeconds * 1000.0f;
  TacProfileSystem* system = TacProfileSystem::Instance;
  TacAssert(system->mCurrStackFrame.back() == mFunction);
  system->mCurrStackFrame.pop_back();
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


  return result;
}

void TacProfileSystem::OnFrameBegin()
{
  Dealloc( mLastFrame );
  mLastFrame = mCurrFrame;
  mCurrFrame = nullptr;
}

void TacProfileSystem::Dealloc( TacProfileFunction* profileFunction )
{
  if( !profileFunction )
    return;
  Dealloc( profileFunction->mChildren );
  profileFunction->mChildren = nullptr;
  Dealloc( profileFunction->mNext );
  profileFunction->mNext = nullptr;
  profileFunction->Clear();
  mFree.push_back( profileFunction );
}

void TacProfileSystem::PushFunction( TacProfileFunction* profileFunction )
{
  if( mCurrStackFrame.empty() )
  {
    mCurrFrame = profileFunction;
  }
  else
  {
    mCurrStackFrame.back()->AppendChild( profileFunction );
  }

  mCurrStackFrame.push_back( profileFunction );
}
