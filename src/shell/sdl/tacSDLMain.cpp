#include "shell/sdl/tacSDLApp.h"
#include "common/tacErrorHandling.h"
#include "common/tacOS.h"

int main( int argc, char **argv )
{
  ( new TacSDLApp )->Run();
  return 0;
}

