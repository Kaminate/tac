#include "tac-std-lib/profile/tac_profile_backend.h"
#include "tac-std-lib/profile/tac_profile.h"
#include "tac-std-lib/profile/tac_profile_function_pool.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{

  const float kProfileStoreSeconds = 0.1f;

  // The function corresponding to the most recent unended ProfileBlockBegin() 
  // Following the parent chain forms the current call stack.
  thread_local ProfileFunction*    sFunctionUnfinished = nullptr;

  static bool                      sIsRunning;
  static Timepoint          sGameFrameTimepointCurr;
  static Timepoint          sGameFrameTimepointPrev;

  struct ProfiledFunctions
  {
    struct Scope
    {
      Scope() : mLockGuard( sProfiledFunctionsMutex ), mProfileFrame( sProfiledFunctions )
      {
      }

      ProfileFrame&                 mProfileFrame;
      std::lock_guard< std::mutex > mLockGuard;
    };

  private:
    static ProfileFrame         sProfiledFunctions;
    static std::mutex           sProfiledFunctionsMutex;
  };

  ProfileFrame         ProfiledFunctions::sProfiledFunctions;
  std::mutex           ProfiledFunctions::sProfiledFunctionsMutex;


  //void              ProfileFunctionVisitor::Visit( ProfileFunction* profileFunction )
  //{
  //  if( profileFunction )
  //    (*this)( profileFunction );

  //  if( profileFunction->mChildren )
  //    (*this)( profileFunction->mChildren );

  //  if( profileFunction->mNext )
  //    (*this)( profileFunction->mNext );
  //}


  // -----------------------------------------------------------------------------------------------

  void ProfileFrame::Clear()
  {
    for( PerThreadProfileFrame& perThreadProfileFrame : mThreadFrames )
      perThreadProfileFrame.Clear();

    mThreadFrames.clear();
  }

  void PerThreadProfileFrame::Clear()
  {
    for( ProfileFunction* fn : mFunctions )
      ProfileFunctionPool::sFunctionPool.Dealloc( fn );

    mFunctions.clear();
  }

  void PerThreadProfileFrame::DeepCopy( const PerThreadProfileFrame& other )
  {
    Clear();
    mThreadId = other.mThreadId;
    for( ProfileFunction* fn : other.mFunctions )
    {
      mFunctions.push_back( ProfileFunctionPool::sFunctionPool.AllocCopy( fn ) );
    }
  }

  //void ProfileFrame::operator = ( const ProfileFrame& other )
  //{
    //Clear();
    //for( const PerThreadProfileFrame& perThread : other.mThreadFrames )
    //{
    //  mThreadFrames.
    //}
  //}

  void ProfileFrame::operator = ( ProfileFrame&& other ) noexcept
  {
    mThreadFrames = other.mThreadFrames;
    //other.mThreadFrames
    OS::OSDebugBreak(); // todooooooooooooo
  }

  ProfileFrame&& ProfileCopyFrame()
  {
    ProfileFrame frameCopy;
    if( !sIsRunning )
      return move(frameCopy);


    ProfiledFunctions::Scope scope;
    frameCopy = scope.mProfileFrame;

    return move( frameCopy );
  }

  void              ProfileSetIsRuning( const bool isRunning ) { sIsRunning = isRunning; }

  bool              ProfileGetIsRuning() { return sIsRunning; }


  void              ProfileSetGameFrame()
  {
    sGameFrameTimepointPrev = sGameFrameTimepointCurr;
    sGameFrameTimepointCurr = Timepoint::Now();
  }

  Timepoint  ProfileTimepointGetLastGameFrameBegin() { return sGameFrameTimepointPrev; }

  ProfileBlock::ProfileBlock( const char* name )
  {
    mName = name;
    mIsActive = sIsRunning;
    if( !mIsActive )
      return;
    
    ProfileFunction* function = ProfileFunctionPool::sFunctionPool.Alloc();
    function->mName = name;
    function->mBeginTime = Timepoint::Now();

    if( sFunctionUnfinished )
      sFunctionUnfinished->AppendChild( function );
    sFunctionUnfinished = function;
  }

  ProfileBlock::~ProfileBlock()
  {
    if( !mIsActive )
      return;

    TAC_ASSERT( sFunctionUnfinished );

    // assume same string literal, dont strcmp. <-- hmm Tac::StringLiteral time?
    TAC_ASSERT( sFunctionUnfinished->mName == mName );

    sFunctionUnfinished->mEndTime = Timepoint::Now();

    if( sFunctionUnfinished->mParent )
    {
      sFunctionUnfinished = sFunctionUnfinished->mParent;
      return;
    }

    // there should be one mutex per thread?
      ProfiledFunctions::Scope scope;
      PerThreadProfileFrame* perThread = scope.mProfileFrame.Find( std::this_thread::get_id() );

      // Since we just added a top-level function, use this opportunity to clean old data.
      perThread->RemoveOldFunctions();

      //ProfileFrame& frame = scope.mProfileFrame;

      //const std::thread::id id = std::this_thread::get_id();


      // Deallocate old profile function memory
      //ProfiledFunctionList& list = sProfiledFunctions[ id ];
      //Vector< ProfileFunction* >& list = mFunctions;
      //ProfiledFunctionList::Iterator it = list.begin();
      //int i = 0;
      //int n = 
      //while( i < n )
      //{
      //  ProfileFunction* profileFunction = *it;
      //  const float secondsAgo = sFunctionUnfinished->mEndTime - profileFunction->mBeginTime;
      //  if( secondsAgo > kProfileStoreSeconds )
      //  {
      //    ProfileFunctionPool::sFunctionPool.Dealloc( profileFunction );
      //    it = list.Erase( it );
      //    i++;
      //  }
      //  else
      //  {
      //    break;
      //  }
      //}
      perThread->mFunctions.push_back( sFunctionUnfinished );
      sFunctionUnfinished = nullptr;

  }

  PerThreadProfileFrame* ProfileFrame::Find( std::thread::id id )
  {
    for(PerThreadProfileFrame& perThreadData : mThreadFrames )
      if( perThreadData.mThreadId == id )
        return &perThreadData;
    return nullptr;
  }

  void PerThreadProfileFrame::RemoveOldFunctions()
  {
    auto now = Timepoint::Now();
    for( auto it = mFunctions.begin();
         it != mFunctions.end();
         it = mFunctions.erase( it ) )
    {
        ProfileFunction* profileFunction = *it;
        const float secondsAgo = now - profileFunction->mBeginTime;

        // Don't need to iterate the remaining functions, the list is chronological
        if( secondsAgo < kProfileStoreSeconds )
          return;

        ProfileFunctionPool::sFunctionPool.Dealloc( profileFunction );
    }
  }
} // namespace Tac

