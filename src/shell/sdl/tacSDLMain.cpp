#include "src/shell/sdl/tacSDLApp.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacOS.h"

int main( int argc, char **argv )
{
  ( new Tac::SDLApp )->Run();
  return 0;
}

