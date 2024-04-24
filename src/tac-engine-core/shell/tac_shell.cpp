#include "tac_shell.h" // self-inc

#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"

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

#if TAC_FONT_ENABLED()
    FontApi::Uninit();
#endif

    //delete mLog;

    ModelAssetManagerUninit();

    // last, so resources can be freed
    Render::RenderApi::Uninit();
  }

  void            ShellInit( Errors& errors )
  {
    JobQueueInit();

    ModelAssetManagerInit();

    TAC_CALL( LocalizationLoad( "assets/localization.txt", errors ));

    TAC_CALL( Render::DefaultCBufferPerFrame::Init( errors ) );
    TAC_CALL( Render::DefaultCBufferPerObject::Init( errors ) );
    TAC_CALL( Render::CBufferLights::Init( errors ) );

    TAC_CALL( UI2DCommonDataInit( errors ));

    TAC_CALL( Debug3DCommonDataInit( errors ));
  }

  String           sShellAppName;
  String           sShellStudioName;
  Filesystem::Path sShellPrefPath; // Path where the app can save files to
  Filesystem::Path sShellInitialWorkingDir;

  AssetPathStringView     ModifyPathRelative( const Filesystem::Path& path, Errors& errors )
  {
    const Filesystem::Path& workingDir { sShellInitialWorkingDir };
    const String workingUTF8 { workingDir.u8string() };

    String pathUTF8 { path.u8string() };
    if( path.is_absolute() )
    {
      if( !pathUTF8.starts_with( workingUTF8 ) )
      {
        const String msg { pathUTF8 + String( " is not in " ) + workingUTF8 };
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
    Filesystem::Path suggestedFilename { assetSaveDialogParams.mSuggestedFilename };
    const OS::SaveParams saveParams
    {
      .mSuggestedFilename = &suggestedFilename,
    };
    const Filesystem::Path fsPath { OS::OSSaveDialog( saveParams, errors ) };
    return ModifyPathRelative( fsPath, errors );
  }

} // namespace Tac
