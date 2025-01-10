#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct JPPTApp : App
  {
    struct JPPTState : IState
    {
    };

    JPPTApp( Config );

    void    Init( Errors& ) override;
    void    Update( Errors& ) override;
    void    Render( RenderParams, Errors& ) override;
    void    Present( Errors& ) override;
    void    Uninit( Errors& ) override;
    State   GameState_Create() override;
    void    GameState_Update( IState* ) override;

  private:
    void    CreateTexture( Errors& );
    void    CreatePipeline( Errors& );
  };
}
