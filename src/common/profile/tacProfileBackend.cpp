#include "src/common/profile/tacProfileBackend.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/string/tacString.h"

namespace Tac
{
  struct ProfileFunctionPool
  {
    ProfileFunction*           Alloc();
    void                       Dealloc( ProfileFunction* );
    Vector< ProfileFunction* > sFunctionsUnused;
  };

  //                               The function corresponding to the most recent unended ProfileBlockBegin() 
  //                               Following the parent chain forms the current call stack.
  thread_local ProfileFunction*    sFunctionUnfinished = nullptr;
  thread_local ProfileFunctionPool sFunctionPool;
  static bool                      sIsRunning;
  static ProfiledFunctions         sProfiledFunctions;
  static std::mutex                sProfiledFunctionsMutex;
  static ProfileTimepoint          sGameFrameTimepointCurr;
  static ProfileTimepoint          sGameFrameTimepointPrev;

  void              ProfileFunctionVisitor::Visit( ProfileFunction* profileFunction )
  {
    if( profileFunction )
      (*this)( profileFunction );
    if( profileFunction->mChildren )
      (*this)( profileFunction->mChildren );
    if( profileFunction->mNext )
      (*this)( profileFunction->mNext );
  }

  ProfileFunction*  ProfileFunctionPool::Alloc()
  {
    if( sFunctionsUnused.empty() )
      return TAC_NEW ProfileFunction;
    ProfileFunction* result = sFunctionsUnused.back();
    sFunctionsUnused.pop_back();
    return result;
  }

  void              ProfileFunctionPool::Dealloc( ProfileFunction* profileFunction )
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

  static void       ProfileFunctionAppendChild( ProfileFunction* parent,
                                                ProfileFunction* child )
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
    }
    child->mParent = parent;
  }

  static ProfileFunction* DeepCopy( const ProfileFunction* profileFunction )
  {
    ProfileFunction* profileFunctionCopy = sFunctionPool.Alloc();
    ProfileFunction* firstChildCopy = nullptr;
    ProfileFunction* prevChildCopy = nullptr;
    for( const ProfileFunction* child = profileFunction->mChildren; child; child = child->mNext )
    {
      ProfileFunction* childCopy = DeepCopy( child );
      childCopy->mParent = profileFunctionCopy;

      if( prevChildCopy )
        prevChildCopy->mNext = childCopy;

      if( !firstChildCopy )
        firstChildCopy = childCopy;

      prevChildCopy = childCopy;
    }

    profileFunctionCopy->mChildren = firstChildCopy;
    profileFunctionCopy->mBeginTime = profileFunction->mBeginTime;
    profileFunctionCopy->mEndTime = profileFunction->mEndTime;
    profileFunctionCopy->mName = profileFunction->mName;
    return profileFunctionCopy;
  }

  void              ProfiledFunctionFree( ProfiledFunctions& profiledFunctions )
  {
    for( auto& pair : profiledFunctions )
    {
      const ProfiledFunctionList& profiledFunctionList = pair.second;
      for( ProfileFunction* profileFunction : profiledFunctionList )
      {
        sFunctionPool.Dealloc( profileFunction );
      }
    }

    profiledFunctions.clear();
  }

  ProfiledFunctions ProfiledFunctionCopy()
  {
    if( !sIsRunning )
      return {};
    ProfiledFunctions profiledFunctionsCopy;
    sProfiledFunctionsMutex.lock();
    for( auto& pair : sProfiledFunctions )
    {
      ProfiledFunctionList& src = pair.second;
      ProfiledFunctionList& dst = profiledFunctionsCopy[ pair.first ];
      for( ProfileFunction* profileFunction : src )
      {
        ProfileFunction* profileFunctionCopy = DeepCopy( profileFunction );
        dst.push_back( profileFunctionCopy );
      }
    }
    sProfiledFunctionsMutex.unlock();
    return profiledFunctionsCopy;
  }

  void              ProfileSetIsRuning( const bool isRunning ) { sIsRunning = isRunning; }

  bool              ProfileGetIsRuning() { return sIsRunning; }

  ProfileTimepoint  ProfileTimepointGet() { return ProfileClock::now(); }

  float             ProfileTimepointSubtract( ProfileTimepoint a, ProfileTimepoint b )
  {
    return ( float )( a - b ).count() / ( float )1e9;
  }

  ProfileTimepoint  ProfileTimepointAddSeconds( ProfileTimepoint profileTimepoint, float seconds )
  {
    return profileTimepoint += std::chrono::nanoseconds( ( int )( seconds * 1e9 ) );
  }

  void              ProfileSetGameFrame()
  {
    sGameFrameTimepointPrev = sGameFrameTimepointCurr;
    sGameFrameTimepointCurr = ProfileTimepointGet();
  }

  ProfileTimepoint  ProfileTimepointGetLastGameFrameBegin()
  {
    return sGameFrameTimepointPrev;
  }

  ProfileBlock::ProfileBlock( const char* name )
  {
    mName = name;
    mIsActive = sIsRunning;
    if( !mIsActive )
      return;
    
    ProfileFunction* function = sFunctionPool.Alloc();
    function->mName = name;
    function->mBeginTime = ProfileTimepointGet(); // ShellGetElapsedSeconds();
    if( sFunctionUnfinished )
      ProfileFunctionAppendChild( sFunctionUnfinished, function );
    sFunctionUnfinished = function;
  }

  ProfileBlock::~ProfileBlock()
  {
    if( !mIsActive )
      return;

    TAC_ASSERT( sFunctionUnfinished );
    TAC_ASSERT( sFunctionUnfinished->mName == mName ); // assume same string literal, dont strcmp

    sFunctionUnfinished->mEndTime = ProfileTimepointGet();
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
        const double secondsAgo = ProfileTimepointSubtract( sFunctionUnfinished->mEndTime,
                                                            profileFunction->mBeginTime );
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

