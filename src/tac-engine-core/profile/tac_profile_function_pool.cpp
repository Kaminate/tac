#include "tac-engine-core/profile/tac_profile_function_pool.h"


namespace Tac
{
  thread_local ProfileFunctionPool ProfileFunctionPool::sFunctionPool;

  ProfileFunction*  ProfileFunctionPool::Alloc()
  {
    if( mFunctionsUnused.empty() )
      return TAC_NEW ProfileFunction;
    ProfileFunction* result { mFunctionsUnused.back() };
    mFunctionsUnused.pop_back();
    return result;
  }

  ProfileFunction* ProfileFunctionPool::AllocCopy( const ProfileFunction* orig )
  {
    ProfileFunction* copy { ProfileFunctionPool::sFunctionPool.Alloc() };
    copy->DeepCopy( orig );
    return copy;
  }

  void              ProfileFunctionPool::Dealloc( ProfileFunction* profileFunction )
  {
    if( !profileFunction )
      return;

    Dealloc( profileFunction->mChildren );
    profileFunction->mChildren = nullptr;
    TAC_ASSERT( !profileFunction->mChildren );

    Dealloc( profileFunction->mNext );
    profileFunction->mNext = nullptr;
    TAC_ASSERT( !profileFunction->mNext );

    *profileFunction = ProfileFunction();
    mFunctionsUnused.push_back( profileFunction );
  }
}

