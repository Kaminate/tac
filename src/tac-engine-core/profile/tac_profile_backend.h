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

  // todo: rename PerThreadProfileData
  struct PerThreadProfileFrame
  {
    ~PerThreadProfileFrame();
    void Clear();
    void DeepCopy( const PerThreadProfileFrame& );
    void RemoveOldFunctions();
    bool empty() const;
    void operator = ( const PerThreadProfileFrame& );

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
    bool empty() const;
    auto Find( std::thread::id ) -> PerThreadProfileFrame*;
    void operator = ( const ProfileFrame& );
    void operator = ( ProfileFrame&& ) noexcept;
    Vector< PerThreadProfileFrame > mThreadFrames;
  };


  auto ProfileRealTimeGetLastGameFrameBegin() -> RealTime;
  auto ProfileCopyFrame() -> ProfileFrame&&;
}

