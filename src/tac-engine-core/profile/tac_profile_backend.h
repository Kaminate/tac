#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-engine-core/profile/tac_profile_function.h"

#if TAC_SHOULD_IMPORT_STD()
import std; // <thread>
#else
#include <thread>
#endif

namespace Tac
{

  //struct ProfileFunctionVisitor
  //{
  //  void             Visit( ProfileFunction* );
  //  virtual void     operator()( ProfileFunction* ) = 0;
  //};

  // todo: rename PerThreadProfileData
  struct PerThreadProfileFrame
  {
    ~PerThreadProfileFrame() { Clear(); }

    void Clear();
    void DeepCopy( const PerThreadProfileFrame& );

    void operator = ( const PerThreadProfileFrame& other ) { DeepCopy( other ); }
    void RemoveOldFunctions();

    bool empty() { return mFunctions.empty(); }

    // There are no 'frames', these are top-level functions added by
    // TAC_PROFILE_BLOCK/TAC_PROFILE_BLOCK_NAMED. By "top-level", we just mean that these
    // functions have no parent function
    //
    // So this list may contain the last kProfileStoreSeconds worth of top level functions.
    // If the entire frame is underneath a single TAC_PROFILE_BLOCK, then the size of this list
    // is the number of currently stored frames, although this doesn't necessarily have to be
    // the case.
    List< ProfileFunction* > mFunctions;
    std::thread::id          mThreadId;
  };

  // todo: rename ProfileData
  struct ProfileFrame
  {
    ProfileFrame() = default;
    ProfileFrame( ProfileFrame& ) = default;
    void Clear();
    bool empty()
    {
      for( PerThreadProfileFrame& f : mThreadFrames )
        if( !f.empty() )
          return false;
      return true;
    }

    void operator = ( const ProfileFrame& other )
    {
      mThreadFrames = other.mThreadFrames;
    }

    void operator = ( ProfileFrame&& ) noexcept;
    PerThreadProfileFrame* Find( std::thread::id );

    //void operator = ( const ProfileFrame& other );
    Vector< PerThreadProfileFrame > mThreadFrames;
  };


  Timepoint           ProfileTimepointGetLastGameFrameBegin();
  ProfileFrame&&      ProfileCopyFrame();
}

