#include "src/shell/sdl/tacsdllib/tac_sdl_app.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_os.h"

using namespace Tac;

void mainAux( Errors& errors )
{
  TAC_CALL( SDLOSInit, errors );
  TAC_CALL( SDLAppInit, errors );
  TAC_CALL( DesktopAppRun, errors );
}

int main( int, char ** )
{
  Errors& errors = GetMainErrors();
  mainAux( errors );
  DesktopAppReportErrors();

  return 0;
}


