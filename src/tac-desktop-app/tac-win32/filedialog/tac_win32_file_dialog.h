#pragma once

#include "tac-std-lib/filesystem/tac_filesystem.h" // FileSys::Path
#include "tac-std-lib/os/tac_os.h"


namespace Tac
{
  struct Errors;

  auto Win32FileDialogOpen( const OS::OpenParams&, Errors& ) -> FileSys::Path;
  auto Win32FileDialogSave( const OS::SaveParams&, Errors& ) -> FileSys::Path;

} // namespace Tac

