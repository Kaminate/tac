#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{

  void ProfileBlockBegin( StackFrame );
  void ProfileBlockEnd();
  void ImGuiProfileWidget();

#define TAC_PROFILE_BLOCK ProfileBlockBegin( TAC_STACK_FRAME ); TAC_ON_DESTRUCT( ProfileBlockEnd() );



}

