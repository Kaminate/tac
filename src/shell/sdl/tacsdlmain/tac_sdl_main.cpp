#include "src/shell/sdl/tacsdllib/tac_sdl_app.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_os.h"

using namespace Tac;

void mainAux( Errors& errors )
{
  SDLOSInit( errors );
  TAC_HANDLE_ERROR( errors );


  SDLAppInit( errors );
  TAC_HANDLE_ERROR( errors );

  DesktopAppRun( errors );
  TAC_HANDLE_ERROR( errors );
}

int main( int, char ** )
{
  Errors& errors = GetMainErrors();
  mainAux( errors );
  DesktopAppReportErrors();

  return 0;
}

