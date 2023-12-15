#pragma once

#include "space/tac_space.h"
#include "src/common/tac_core.h"

namespace Tac
{

  void                          ShadowPresentationInit( Errors& );
  void                          ShadowPresentationUninit();
  void                          ShadowPresentationRender( World* );
  void                          ShadowPresentationDebugImGui( Graphics* );
}

