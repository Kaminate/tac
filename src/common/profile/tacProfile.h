#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{

  //void ProfileBlockBegin( StackFrame );
  //void ProfileBlockEnd();
  void ProfileSetGameFrame();
  void ProfileSetIsRuning( bool );
  bool ProfileGetIsRuning();
  void ImGuiProfileWidget();

  struct ProfileBlock
  {
    ProfileBlock( const char* );
    ~ProfileBlock();
    const char* mName;
    bool        mIsActive;
  };

//#define TAC_PROFILE_BLOCK_NAME_AUX(a,b)    a ## b
//#define TAC_PROFILE_BLOCK_NAME(profile, __LINE__)    TAC_PROFILE_BLOCK_NAME_AUX(profile, __LINE__)
#define TAC_PROFILE_BLOCK               ProfileBlock TAC_CONCAT(profile,__COUNTER__)( __FUNCTION__ );
#define TAC_PROFILE_BLOCK_NAMED( name ) ProfileBlock TAC_CONCAT(profile,__COUNTER__)( name );
//#define TAC_PROFILE_BLOCK ProfileBlockBegin( TAC_STACK_FRAME ); TAC_ON_DESTRUCT( ProfileBlockEnd() );



}

