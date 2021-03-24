#include "src/common/profile/tacProfileBackend.h"
#include "src/common/shell/tacShellTimer.h"

namespace Tac
{
  //===-----------===//
  // ProfileFunction //
  //===-----------===//


  static void             ProfileFunctionAppendChild( ProfileFunction* parent, ProfileFunction* child )
  {
    if( parent->mChildren )
    {
      ProfileFunction* lastChild = parent->mChildren;
      while( lastChild->mNext )
        lastChild = lastChild->mNext;
      lastChild->mNext = child;
    }
    else
    {
      parent->mChildren = child;
      child->mParent = parent;
    }
  }



  //===----------------===//
  // ProfilePerThreadData //
  //===----------------===//

  //                               The function corresponding to the most recent unended ProfileBlockBegin() 
  //                               Following the parent chain forms the current call stack.
  thread_local ProfileFunction*    sFunctionUnfinished = nullptr;
  thread_local ProfileFunctionPool sFunctionPool;


  ProfileFunction*           ProfileFunctionPool::Alloc()
  {
    if( sFunctionsUnused.empty() )
      return TAC_NEW ProfileFunction;
    ProfileFunction* result = sFunctionsUnused.back();
    sFunctionsUnused.pop_back();
    return result;
  }

  void                       ProfileFunctionPool::Dealloc( ProfileFunction* profileFunction )
  {
    if( !profileFunction )
      return;
    Dealloc( profileFunction->mChildren );
    profileFunction->mChildren = nullptr;
    Dealloc( profileFunction->mNext );
    profileFunction->mNext = nullptr;
    TAC_ASSERT( !profileFunction->mChildren );
    TAC_ASSERT( !profileFunction->mNext );
    *profileFunction = ProfileFunction();
    sFunctionsUnused.push_back( profileFunction );
  }

  //===-------===//
  // Profile API //
  //===-------===//

  void ProfileBlockBegin( const StackFrame stackFrame )
  {
    ProfileFunction* function = sFunctionPool.Alloc();
    function->mStackFrame = stackFrame;
    function->mBeginTime = ShellGetElapsedSeconds();
    if( sFunctionUnfinished )
      ProfileFunctionAppendChild( sFunctionUnfinished, function );
    else
      sFunctionUnfinished = function;
  }


  static ProfiledFunctions sProfiledFunctions;
  static std::mutex        sProfiledFunctionsMutex;

  static ProfileFunction* DeepCopy( ProfileFunction* profileFunction, ProfileFunctionPool* profileFunctionPool )
  {
    ProfileFunction* profileFunctionCopy = profileFunctionPool->Alloc();

    ProfileFunction* firstChildCopy = nullptr;
    ProfileFunction* prevChildCopy = nullptr;
    for( ProfileFunction* child = profileFunction->mChildren; child; child = child->mNext )
    {
      ProfileFunction* childCopy = DeepCopy( child, profileFunctionPool );
      childCopy->mParent = profileFunctionCopy;

      if( prevChildCopy )
        prevChildCopy->mNext = childCopy;

      if( !firstChildCopy )
        firstChildCopy = childCopy;
      prevChildCopy = child;
    }

    profileFunctionCopy->mChildren = firstChildCopy;
    profileFunctionCopy->mBeginTime = profileFunction->mBeginTime;
    profileFunctionCopy->mEndTime = profileFunction->mEndTime;
    profileFunctionCopy->mStackFrame = profileFunction->mStackFrame;
    return profileFunctionCopy;
  }

  ProfiledFunctions CopyProfiledFunctions( ProfileFunctionPool* profileFunctionPool )
  {
    ProfiledFunctions profiledFunctionsCopy;
    sProfiledFunctionsMutex.lock();
    for( auto& pair : sProfiledFunctions )
    {
      ProfiledFunctionList& profiledFunctionList = pair.second;
      ProfiledFunctionList& profiledFunctionListCopy = profiledFunctionsCopy[ pair.first ];

      for( ProfileFunction* profileFunction : profiledFunctionList )
      {
        ProfileFunction* profileFunctionCopy = DeepCopy( profileFunction, profileFunctionPool );
        profiledFunctionListCopy.push_back( profileFunctionCopy );
      }
    }
    sProfiledFunctionsMutex.unlock();
    return profiledFunctionsCopy;
  }

  void ProfileBlockEnd()
  {
    sFunctionUnfinished->mEndTime = ShellGetElapsedSeconds();
    if( sFunctionUnfinished->mParent )
    {
      sFunctionUnfinished = sFunctionUnfinished->mParent;
    }
    else
    {
      sProfiledFunctionsMutex.lock();

      const std::thread::id id = std::this_thread::get_id();

      // Deallocate old profile function memory
      ProfiledFunctionList& list = sProfiledFunctions[ id ];
      ProfiledFunctionList::iterator it = list.begin();
      while( it != list.end() )
      {
        ProfileFunction* profileFunction = *it;
        const double secondsAgo = sFunctionUnfinished->mEndTime - profileFunction->mBeginTime;
        if( secondsAgo > kProfileStoreSeconds )
        {
          sFunctionPool.Dealloc( profileFunction );
          it = list.erase( it );
        }
        else
        {
          break;
        }
      }
      list.push_back( sFunctionUnfinished );
      sFunctionUnfinished = nullptr;

      sProfiledFunctionsMutex.unlock();
    }
  }

}

