
#include "src/common/profile/tacProfile.h"
#include "src/common/tacPreprocessor.h"
namespace Tac
{

  ////////////////////////
  // ProfileFunction //
  ////////////////////////

  void ProfileFunction::Clear()
  {
    TAC_ASSERT( !mChildren );
    TAC_ASSERT( !mNext );
    *this = ProfileFunction();
  }

  ProfileFunction* ProfileFunction::GetLastChild()
  {
    if( !mChildren )
      return nullptr;
    ProfileFunction* child = mChildren;
    while( child->mNext )
      child = child->mNext;
    return child;
  }

  void ProfileFunction::AppendChild( ProfileFunction* child )
  {
    ProfileFunction* lastChild = GetLastChild();
    if( lastChild )
      lastChild->mNext = child;
    else
      mChildren = child;
  }

  /////////////////////
  // ProfileBlock //
  /////////////////////

  ProfileBlock::ProfileBlock( StackFrame frame )
  {
    ProfileSystem* system = ProfileSystem::Instance;
    mFunction = system->Alloc();
    mFunction->mFrame = frame;
    mFunction->mBeginTime = Clock::now();
    system->PushFunction( mFunction );
  }

  ProfileBlock::~ProfileBlock()
  {
    mFunction->mEndTime = Clock::now();
    ProfileSystem* system = ProfileSystem::Instance;
    TAC_ASSERT( system->mCurrStack.back() == mFunction );
    system->mCurrStack.pop_back();
  }

  //////////////////////
  // ProfileSystem //
  //////////////////////

  thread_local ProfileSystem* ProfileSystem::Instance = nullptr;
  ProfileSystem::ProfileSystem()
  {
    Instance = this;
  }

  void ProfileSystem::Init()
  {
  }


  ProfileFunction* ProfileSystem::Alloc()
  {
    ProfileFunction* result;
    if( mFree.size() )
    {
      result = mFree.back();
      mFree.pop_back();
    }
    else
    {
      result = new ProfileFunction;
    }


    return result;
  }

  void ProfileSystem::OnFrameBegin()
  {

  }
  void ProfileSystem::OnFrameEnd()
  {
    Dealloc( mLastFrame );
    mLastFrame = mCurrFrame;
    mCurrFrame = nullptr;
  }

  void ProfileSystem::Dealloc( ProfileFunction* profileFunction )
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

  void ProfileSystem::PushFunction( ProfileFunction* profileFunction )
  {
    if( mCurrStack.empty() )
    {
      mCurrFrame = profileFunction;
    }
    else
    {
      mCurrStack.back()->AppendChild( profileFunction );
    }

    mCurrStack.push_back( profileFunction );
  }

}

