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
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"

Tac::String        Tac::Shell::sShellAppName;
Tac::String        Tac::Shell::sShellStudioName;
Tac::FileSys::Path Tac::Shell::sShellPrefPath; // Path where the app can save files to
Tac::FileSys::Path Tac::Shell::sShellInitialWorkingDir;

void Tac::Shell::Uninit()
{
  UI2DCommonDataUninit();
  Debug3DCommonDataUninit();

#if TAC_FONT_ENABLED()
  FontApi::Uninit();
#endif

  //delete mLog;

  ModelAssetManager::Uninit();

  // last, so resources can be freed
  Render::RenderApi::Uninit();
}

void Tac::Shell::Init( Errors& errors )
{
  Job::JobQueueInit();
  ModelAssetManager::Init();
  TAC_CALL( LocalizationLoad( "assets/localization.txt", errors ) );
  TAC_CALL( Render::DefaultCBufferPerFrame::Init( errors ) );
  TAC_CALL( Render::DefaultCBufferPerObject::Init( errors ) );
  TAC_CALL( Render::CBufferLights::Init( errors ) );
  TAC_CALL( UI2DCommonDataInit( errors ) );
  TAC_CALL( Debug3DCommonDataInit( errors ) );
}

auto Tac::ModifyPathRelative( const FileSys::Path& path, Errors& errors ) -> Tac::AssetPathStringView
{
  const FileSys::Path& workingDir { Shell::sShellInitialWorkingDir };
  const String workingUTF8 { workingDir.u8string() };
  String pathUTF8 { path.u8string() };
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

auto Tac::AssetOpenDialog( Errors& errors ) -> Tac::AssetPathStringView
{
  TAC_CALL_RET( const FileSys::Path fsPath{ OS::OSOpenDialog( errors ) } );
  return ModifyPathRelative( fsPath, errors );
}

auto Tac::AssetSaveDialog( const AssetSaveDialogParams& params, Errors& errors ) -> Tac::AssetPathStringView
{
  FileSys::Path suggestedFilename { params.mSuggestedFilename };
  const OS::SaveParams saveParams { .mSuggestedFilename { &suggestedFilename }, };
  const FileSys::Path fsPath { OS::OSSaveDialog( saveParams, errors ) };
  return ModifyPathRelative( fsPath, errors );
}

