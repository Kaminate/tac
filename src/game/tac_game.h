#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct GameApp : public App
  {
    GameApp( const Config& );
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Render( RenderParams, Errors& ) override;
  };
}

