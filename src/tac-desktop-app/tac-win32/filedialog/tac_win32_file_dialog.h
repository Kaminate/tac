#pragma once

#include "tac-std-lib/filesystem/tac_filesystem.h" // UTF8Path
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac
{
  auto Win32FileDialogOpen( const OpenParams&, Errors& ) -> UTF8Path;
  auto Win32FileDialogSave( const SaveParams&, Errors& ) -> UTF8Path;
} // namespace Tac

