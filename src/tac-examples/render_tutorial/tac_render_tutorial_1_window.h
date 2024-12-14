#include "tac_render_tutorial.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct RenderTutorial1Window : public App
  {
    RenderTutorial1Window( App::Config );
    void Init( Errors& ) override;
  };
} // namespace Tac

