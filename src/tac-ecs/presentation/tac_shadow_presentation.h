#pragma once

namespace Tac { struct Errors; struct World; struct Graphics; }
namespace Tac
{
  void                          ShadowPresentationInit( Errors& );
  void                          ShadowPresentationUninit();
  void                          ShadowPresentationRender( World*, Errors& );
  void                          ShadowPresentationDebugImGui( Graphics* );
}

