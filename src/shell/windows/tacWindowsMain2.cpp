// Entry point into a windows application

#include "shell/windows/tacWindowsApp2.h"
#include "common/tacOS.h"


int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow )
{
  TacErrors errors;
  auto windowsApplication = new TacWindowsApplication2();
  windowsApplication->mHInstance = hInstance;
  windowsApplication->mlpCmdLine = lpCmdLine;
  windowsApplication->mNCmdShow = nCmdShow;
  windowsApplication->mhPrevInstance = hPrevInstance;
  windowsApplication->Init( errors );
  TacDesktopApp::DoStuff( windowsApplication, errors );
  if( errors.size() )
    TacOS::Instance->DebugPopupBox( errors.ToString() );
  return 0;
}
