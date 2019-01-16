#include "shell/sdl/tacSDLApp.h"
#include "common/tacErrorHandling.h"
#include "common/tacOS.h"

int main( int argc, char **argv )
{
  TacErrors errors;
  auto app = new TacSDLApp;
  app->Init( errors );
  TacDesktopApp::DoStuff( app, errors );
  if( errors.size() )
    TacOS::Instance->DebugPopupBox( errors.ToString() );
  return 0;
}

