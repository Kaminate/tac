#include "tac_resident_model_file.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"

namespace Tac
{
  struct LoadJob : public Job
  {
    void ExecuteAux( Errors& );
    void Execute( Errors& ) override;
    void Clear();

    String            mBytes      {};
    UTF8Path mPath       {};
    cgltf_data*       mParsedData {};
  };

  // -----------------------------------------------------------------------------------------------

  void LoadJob::ExecuteAux( Errors& errors )
  {
    const cgltf_options options{};
    TAC_CALL( mBytes = mPath.LoadFilePath( errors ) );
    TAC_GLTF_CALL( cgltf_parse, &options, mBytes.data(), mBytes.size(), &mParsedData );
    TAC_GLTF_CALL( cgltf_validate, mParsedData );
  }

  void LoadJob::Execute( Errors& errors )
  {
    for(;;)
    {
      ExecuteAux( errors );
      if( !errors )
        break;

      if( kIsDebugMode )
      {
        OS::OSDebugBreak();
        errors.clear();
      }
    }
  }

  void LoadJob::Clear()
  {
    mBytes.clear();
    mPath.clear();
    cgltf_free( mParsedData );
    mParsedData = nullptr;
    Job::Clear();
  }

  // -----------------------------------------------------------------------------------------------

  struct LoadedStuff
  {
    void             Clear();

    UTF8Path    mPath               {};
    StringID         mAssetPathID        {};
    GameTime         mLastRequestSeconds {};
    cgltf_data*      mParsedData         {};
  };

  // -----------------------------------------------------------------------------------------------

  void LoadedStuff::Clear()
  {
    mPath.clear();
    mLastRequestSeconds = {};
    cgltf_free( mParsedData );
    mParsedData = nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  struct LoadingStuff
  {
    void             Clear();
    

    UTF8Path    mPath;
    StringID         mAssetPathID;
    LoadJob          mJob;
  };

  // -----------------------------------------------------------------------------------------------

  void LoadingStuff::Clear()
  {
    mPath.clear();
    mJob.Clear();
    mAssetPathID = {};
  }

  // -----------------------------------------------------------------------------------------------

  static LoadingStuff sLoadingStuff[ 4 ];
  static LoadedStuff  sLoadedStuff[ 4 ];

  static auto TryGetEmptyLoadingStuff() -> LoadingStuff*
  {
    for( LoadingStuff& loadingStuff : sLoadingStuff )
      if( loadingStuff.mPath.empty() )
        return &loadingStuff;
    return nullptr;
  }

  static auto TryGetEmptyLoadedStuff() -> LoadedStuff*
  {
    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( loadedStuff.mPath.empty() )
        return &loadedStuff;
    return nullptr;
  }

  static void ResidentModelFileUpdate()
  {
    static GameTime lastUpdateSeconds;
    const GameTime currUpdateSeconds { GameTimer::GetElapsedTime() };
    if( lastUpdateSeconds == currUpdateSeconds )
      return;

    lastUpdateSeconds = currUpdateSeconds;

    const GameTimeDelta persistSeconds { 1.0f };

    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( loadedStuff.mLastRequestSeconds + persistSeconds > currUpdateSeconds )
        loadedStuff.Clear();

    for( LoadingStuff& loadingStuff : sLoadingStuff )
    {
      if( loadingStuff.mJob.GetStatus() != JobState::ThreadFinished )
        continue;

      if( loadingStuff.mJob.mErrors )
      {
        if constexpr( kIsDebugMode )
          OS::OSDebugBreak();
        loadingStuff.Clear();
        continue;
      }

      LoadedStuff* loadedStuff { TryGetEmptyLoadedStuff() };
      if( !loadedStuff )
        continue;

      loadedStuff->mPath = loadingStuff.mPath;
      loadedStuff->mLastRequestSeconds = currUpdateSeconds;
      loadedStuff->mParsedData = loadingStuff.mJob.mParsedData;
      loadedStuff->mAssetPathID = loadingStuff.mAssetPathID;
      loadingStuff.mJob.mParsedData = nullptr;
      loadingStuff.Clear();
    }
  }

  static auto FindLoadedStuff( StringID id ) -> LoadedStuff*
  {
    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( loadedStuff.mAssetPathID == id )
        return &loadedStuff;
    return nullptr;
  }

  static auto FindLoadingStuff( StringID id ) -> LoadingStuff*
  {
    for( LoadingStuff& stuff : sLoadingStuff )
      if( stuff.mAssetPathID == id )
        return &stuff;
    return nullptr;
  }


} // namespace Tac

auto Tac::TryGetGLTFData( const AssetPathStringView& assetPath ) -> const cgltf_data*
{
  ResidentModelFileUpdate();

  const StringID id{ assetPath };

  if( LoadedStuff * stuff{ FindLoadedStuff( id ) } )
  {
      stuff->mLastRequestSeconds = GameTimer::GetElapsedTime();
      return stuff->mParsedData;
  }

  // Still loading
  if( FindLoadingStuff( id ) )
    return nullptr;

  LoadingStuff* loadingStuff { TryGetEmptyLoadingStuff() };
  if( !loadingStuff )
    return nullptr;

  loadingStuff->mPath = assetPath;
  loadingStuff->mAssetPathID = id;
  loadingStuff->mJob.mPath = assetPath;
  Job::JobQueuePush( &loadingStuff->mJob );

  return nullptr;
}
