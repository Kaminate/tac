#pragma once

#include "tac-std-lib/containers/tac_span.h"

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct ContextHandle;
  struct ViewHandle2;

  // -----------------------------------------------------------------------------------------------

  struct InitParams
  {
    int mMaxGPUFrameCount = 2;
  };

  void              Init2( InitParams, Errors& );
  int               GetMaxGPUFrameCount();
  void              ExecuteCommands( Span< ContextHandle >, Errors& );
  ContextHandle     CreateContext( Errors& );
  ViewHandle2       CreateView2();

  //Render::CreateFramebufferForWindow( nativeWindowHandle, w, h, TAC_STACK_FRAME );

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render

