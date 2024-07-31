#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct JPPTApp : App
  {
    JPPTApp( Config );
    void    Init( InitParams, Errors& )       override;
    void    Update( UpdateParams, Errors& )   override;
    void    Render( RenderParams, Errors& )   override;
    void    Present( PresentParams, Errors& ) override;
    void    Uninit( Errors& )                 override;
  };
}
