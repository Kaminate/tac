#include "tac_settings_root.h" // self-inc

#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{
  void SettingsRoot::Init( const UTF8Path& path , Errors& errors )
  {
    sSavePath = path;
    if( sSavePath.Exists() )
    {
      const String loaded{ sSavePath.LoadFilePath( errors ) };
      sJson = Json::Parse( loaded, errors );
    }
  }

  auto SettingsRoot::GetRootNode() -> SettingsNode { return SettingsNode( this, &sJson ); }

  void SettingsRoot::Tick( Errors& errors )
  {
    if( !sDirty )
      return;

    const GameTime elapsedSeconds{ GameTimer::GetElapsedTime() };
    const GameTimeDelta saveFrequencySecs{ 0.1f };
    const bool savedRecently{ elapsedSeconds < sLastSaveSeconds + saveFrequencySecs };
    if( savedRecently )
      return;

    Flush( errors );
  }

  void SettingsRoot::Flush( Errors& errors )
  {
    if( !sDirty )
      return;

    const String str{ sJson.Stringify() };
    const void* data{ ( void* )str.data() };
    const int n{ ( int )str.size() };
    TAC_CALL( sSavePath.SaveToFile( data, n, errors ) );
    sDirty = false;
    sLastSaveSeconds = GameTimer::GetElapsedTime();
  }

  void SettingsRoot::SetDirty() { sDirty = true; }
}

