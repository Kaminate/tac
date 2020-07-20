// Entry point into a windows application

#include "src/shell/windows/tacWindowsApp2.h"

int CALLBACK WinMain( HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow )
{
  TAC_NEW Tac::WindowsApplication2( hInstance,
                                    hPrevInstance,
                                    lpCmdLine,
                                    nCmdShow );
  Tac::WindowsApplication2::Instance->Run();
  delete Tac::WindowsApplication2::Instance;
  return 0;
}
