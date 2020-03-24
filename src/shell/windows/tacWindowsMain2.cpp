// Entry point into a windows application

#include "shell/windows/tacWindowsApp2.h"
#include "common/tacOS.h"

// temp
#include "common/tacJson.h"


int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow )
{
  auto windowsApplication = new TacWindowsApplication2();
  windowsApplication->mHInstance = hInstance;
  windowsApplication->mlpCmdLine = lpCmdLine;
  windowsApplication->mNCmdShow = nCmdShow;
  windowsApplication->mhPrevInstance = hPrevInstance;
  windowsApplication->Run();
  delete windowsApplication;

  return 0;
}
