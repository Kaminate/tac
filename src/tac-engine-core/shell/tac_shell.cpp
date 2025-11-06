#include "tac_shell.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"

Tac::String        Tac::Shell::sShellAppName;
Tac::String        Tac::Shell::sShellStudioName;
Tac::FileSys::Path Tac::Shell::sShellPrefPath; // Path where the app can save files to
Tac::FileSys::Path Tac::Shell::sShellInitialWorkingDir;

auto Tac::ModifyPathRelative( const FileSys::Path& path, Errors& errors ) -> Tac::AssetPathStringView
{
  const FileSys::Path& workingDir { Shell::sShellInitialWorkingDir };
  const String workingUTF8 { workingDir.u8string() };
  dynmc String pathUTF8 { path.u8string() };
  if( path.is_absolute() )
  {
    TAC_RAISE_ERROR_IF_RETURN( !pathUTF8.starts_with( workingUTF8 ), 
                               String() + pathUTF8 + String( " is not in " ) + workingUTF8 );
    pathUTF8.erase( 0, workingUTF8.size() );
    pathUTF8 = FileSys::StripLeadingSlashes( pathUTF8 );
  }

  pathUTF8.replace( "\\", "/");
  return FrameMemoryCopy( pathUTF8.c_str() );
}


