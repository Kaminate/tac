#include "tac-std-lib/profile/tac_profile_function.h"
#include "tac-std-lib/profile/tac_profile_function_pool.h"

namespace Tac
{

  void       ProfileFunction::AppendChild( ProfileFunction* child )
  {
    child->mParent = this;

    if( !mChildren )
    {
      mChildren = child;
      return;
    }

    ProfileFunction* lastChild = mChildren;
    while( lastChild->mNext )
      lastChild = lastChild->mNext;

    lastChild->mNext = child;
  }

  void ProfileFunction::DeepCopy( const ProfileFunction* profileFunction )
  {
    ProfileFunction* profileFunctionCopy = this;

    ProfileFunction* firstChildCopy = nullptr;
    ProfileFunction* prevChildCopy = nullptr;
    for( const ProfileFunction* child = profileFunction->mChildren;
         child;
         child = child->mNext )
    {
      ProfileFunction* childCopy = ProfileFunctionPool::sFunctionPool.AllocCopy( child );
      childCopy->mParent = this;

      if( prevChildCopy )
        prevChildCopy->mNext = childCopy;

      if( !firstChildCopy )
        firstChildCopy = childCopy;

      prevChildCopy = childCopy;
    }

    mChildren = firstChildCopy;
    mBeginTime = profileFunction->mBeginTime;
    mEndTime = profileFunction->mEndTime;
    mName = profileFunction->mName;
  }
}

