#pragma once

#include "src/space/tac_space.h"
#include "src/common/tac_common.h"

namespace Tac
{

  void                          ShadowPresentationInit( Errors& );
  void                          ShadowPresentationUninit();
  void                          ShadowPresentationRender( World* );
  void                          ShadowPresentationDebugImGui( Graphics* );
}

