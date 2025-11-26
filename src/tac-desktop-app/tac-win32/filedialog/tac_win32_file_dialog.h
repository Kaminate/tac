#pragma once

#include "tac-std-lib/filesystem/tac_filesystem.h" // UTF8Path
#include "tac-std-lib/os/tac_os.h"


namespace Tac
{
  struct Errors;

  auto Win32FileDialogOpen( const OS::OpenParams&, Errors& ) -> UTF8Path;
  auto Win32FileDialogSave( const OS::SaveParams&, Errors& ) -> UTF8Path;

} // namespace Tac

