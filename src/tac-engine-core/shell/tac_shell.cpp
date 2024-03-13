#include "tac-std-lib/shell/tac_shell.h" // self-include

#include "tac-std-lib/assetmanagers/tac_model_asset_manager.h"
#include "tac-std-lib/assetmanagers/tac_asset.h"
#include "tac-std-lib/assetmanagers/tac_texture_asset_manager.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/dataprocess/tac_settings.h"
#include "tac-rhi/debug/tac_debug_3d.h"
#include "tac-rhi/ui/tac_font.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/tac_renderer_util.h"
#include "tac-rhi/ui/tac_ui_2d.h"
#include "tac-std-lib/input/tac_controller_input.h"
#include "tac-std-lib/input/tac_keyboard_input.h"
#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/net/tac_net.h"
#include "tac-std-lib/profile/tac_profile.h"
#include "tac-std-lib/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/assetmanagers/tac_asset.h"
#include "tac-std-lib/system/tac_filesystem.h"
#include "tac-std-lib/system/tac_job_queue.h"
#include "tac-std-lib/os/tac_os.h"

//#include <iostream>
//#include <thread>

namespace Tac
{
  Soul::Soul()
  {
    mIsImGuiVisible = true;
  }

  void            ShellUninit()
  {
    UI2DCommonDataUninit();
    Debug3DCommonDataUninit();

    FontApi::Uninit();

    //delete mLog;

    ModelAssetManagerUninit();

    // last, so resources can be freed
    Render::Uninit();
  }

  void            ShellInit( Errors& errors )
  {
    JobQueueInit();

    ModelAssetManagerInit();

    TAC_CALL( LocalizationLoad( "assets/localization.txt", errors ));

    Render::DefaultCBufferPerFrame::Init();
    Render::DefaultCBufferPerObject::Init();
    Render::CBufferLights::Init();

    TAC_CALL( UI2DCommonDataInit( errors ));

    TAC_CALL( Debug3DCommonDataInit( errors ));
  }

  String           sShellAppName;
  String           sShellStudioName;
  Filesystem::Path sShellPrefPath; // Path where the app can save files to
  Filesystem::Path sShellInitialWorkingDir;

  AssetPathStringView     ModifyPathRelative( const Filesystem::Path& path, Errors& errors )
  {
    const Filesystem::Path& workingDir = sShellInitialWorkingDir;
    const String workingUTF8 = workingDir.u8string();

    String pathUTF8 = path.u8string();
    if( path.is_absolute() )
    {
      if( !pathUTF8.starts_with( workingUTF8 ) )
      {
        const String msg = pathUTF8 + String( " is not in " ) + workingUTF8;
        TAC_RAISE_ERROR_RETURN( msg, {} );
      }

      pathUTF8.erase( 0, workingUTF8.size() );
      pathUTF8 = Filesystem::StripLeadingSlashes( pathUTF8 );
    }

    for( char& c : pathUTF8 )
      if( c == '\\' )
        c = '/';

    return FrameMemoryCopy( pathUTF8.c_str() );
  }

  AssetPathStringView     AssetOpenDialog( Errors& errors )
  {
    const Filesystem::Path fsPath = TAC_CALL_RET( {}, OS::OSOpenDialog( errors ));

    return ModifyPathRelative( fsPath, errors );
  }

  AssetPathStringView     AssetSaveDialog(
    const AssetSaveDialogParams& assetSaveDialogParams,
    Errors& errors )
  {
    Filesystem::Path suggestedFilename = assetSaveDialogParams.mSuggestedFilename;
    const OS::SaveParams saveParams
    {
      .mSuggestedFilename = &suggestedFilename,
    };
    const Filesystem::Path fsPath = OS::OSSaveDialog( saveParams, errors );
    return ModifyPathRelative( fsPath, errors );
  }

} // namespace Tac
