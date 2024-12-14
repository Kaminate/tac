#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct JPPTApp : App
  {
    JPPTApp( Config );

    void    Init(  Errors& )       override;
    void    Update(  Errors& )   override;
    void    Render( RenderParams, Errors& )   override;
    void    Present(  Errors& ) override;
    void    Uninit( Errors& )                 override;

  private:
    void    CreateTexture( Errors& );
    void    CreatePipeline( Errors& );
  };
}
