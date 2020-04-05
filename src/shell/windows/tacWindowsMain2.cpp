// Entry point into a windows application

#include "src/shell/windows/tacWindowsApp2.h"

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow )
{
  auto windowsApplication = new Tac::WindowsApplication2();
  windowsApplication->mHInstance = hInstance;
  windowsApplication->mlpCmdLine = lpCmdLine;
  windowsApplication->mNCmdShow = nCmdShow;
  windowsApplication->mhPrevInstance = hPrevInstance;
  windowsApplication->Run();
  delete windowsApplication;

  return 0;
}
