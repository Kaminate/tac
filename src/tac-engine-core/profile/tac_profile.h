#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  //   Sets a point of reference to align the profile visulization
  void ProfileSetGameFrame();

  //   Profile data will be collected when running is true
  void ProfileSetIsRuning( bool );
  bool ProfileGetIsRuning();

  //   API to draw the profile visualization
  void ImGuiProfileWidget();

  struct ProfileBlock
  {
    // Starts the profile timer
    ProfileBlock( const char* );

    // Stops the profile timer
    ~ProfileBlock();

    const char* mName;
    bool        mIsActive;
  };


}

//#define TAC_PROFILE_BLOCK               Tac::ProfileBlock TAC_CONCAT( profile, __COUNTER__ )( __FUNCTION__ )
#define TAC_PROFILE_BLOCK_NAMED( name ) Tac::ProfileBlock TAC_CONCAT( profile, __COUNTER__ )( name )
#define TAC_PROFILE_BLOCK               TAC_PROFILE_BLOCK_NAMED( __FUNCTION__ )
