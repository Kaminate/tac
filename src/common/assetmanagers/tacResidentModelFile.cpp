#include "src/common/assetmanagers/tacResidentModelFile.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/thirdparty/cgltf.h"
#include "src/common/string/tacString.h"
#include "src/common/tacJobQueue.h"
#include "src/common/shell/tacShellTimer.h"

namespace Tac
{
  struct LoadJob : public Job
  {
    void          Execute() override
    {
      Errors& errors = mErrors;

      bytes = TemporaryMemoryFromFile( mPath, errors );
      TAC_HANDLE_ERROR( errors );

      const cgltf_options options = {};

      const cgltf_result parseResult = cgltf_parse( &options, bytes.data(), bytes.size(), &mParsedData );
      TAC_HANDLE_ERROR_IF( parseResult != cgltf_result_success,
                           va( "%s cgltf_parse returned %i", mPath.c_str(), ( int )( parseResult ) ),
                           errors );

      const cgltf_result validateResult = cgltf_validate( mParsedData );
      TAC_HANDLE_ERROR_IF( validateResult != cgltf_result_success,
                           va( "%s cgltf_validate returned %i", mPath.c_str(), ( int )( validateResult ) ),
                           errors );
    }

    void          Clear()
    {
      bytes.clear();
      mPath.clear();
      cgltf_free( mParsedData );
      mParsedData = nullptr;
    }

    TemporaryMemory bytes;
    String          mPath;
    cgltf_data*     mParsedData = nullptr;
  };

  struct LoadedStuff
  {
    void        Clear()
    {
      mPath.clear();
      mLastRequestSeconds = 0;
      cgltf_free( mParsedData );
      mParsedData = nullptr;
    }

    String      mPath;
    double      mLastRequestSeconds = 0;
    cgltf_data* mParsedData = nullptr;
  };

  struct LoadingStuff
  {
    void        Clear()
    {
      mPath.clear();
      mJob.Clear();
    }

    String      mPath;
    LoadJob     mJob;
  };

  static LoadingStuff sLoadingStuff[ 4 ];
  static LoadedStuff  sLoadedStuff[ 4 ];

  static LoadingStuff* TryGetEmptyLoadingStuff()
  {
    for( LoadingStuff& loadingStuff : sLoadingStuff )
      if( loadingStuff.mPath.empty() )
        return &loadingStuff;
    return nullptr;
  }

  static LoadedStuff*  TryGetEmptyLoadedStuff()
  {
    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( loadedStuff.mPath.empty() )
        return &loadedStuff;
    return nullptr;
  }

  static void          ResidentModelFileUpdate()
  {
    static double lastUpdateSeconds;
    const double currUpdateSeconds = ShellGetElapsedSeconds();
    if( lastUpdateSeconds == currUpdateSeconds )
      return;
    lastUpdateSeconds = currUpdateSeconds;

    const double persistSeconds = 1.0;

    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( loadedStuff.mLastRequestSeconds + persistSeconds > currUpdateSeconds )
        loadedStuff.Clear();

    for( LoadingStuff& loadingStuff : sLoadingStuff )
    {
      if( loadingStuff.mJob.GetStatus() != JobState::ThreadFinished )
        continue;
      if( loadingStuff.mJob.mErrors )
      {
        loadingStuff.Clear();
        continue;
      }

      LoadedStuff* loadedStuff = TryGetEmptyLoadedStuff();
      if( !loadedStuff )
        continue;
      loadedStuff->mPath = loadingStuff.mPath;
      loadedStuff->mLastRequestSeconds = currUpdateSeconds;
      loadedStuff->mParsedData = loadingStuff.mJob.mParsedData;
      loadingStuff.mJob.mParsedData = nullptr;
      loadingStuff.Clear();
    }
  }

  const cgltf_data*    TryGetGLTFData( const char* path )
  {
    ResidentModelFileUpdate();

    for( LoadedStuff& loadedStuff : sLoadedStuff )
      if( !StrCmp( loadedStuff.mPath, path ) )
      {
        loadedStuff.mLastRequestSeconds = ShellGetElapsedSeconds();
        return loadedStuff.mParsedData;
      }

    for( LoadingStuff& loadingStuff : sLoadingStuff )
      if( loadingStuff.mPath == path )
        return nullptr;

    LoadingStuff* loadingStuff = TryGetEmptyLoadingStuff();
    if( !loadingStuff )
      return nullptr;
    loadingStuff->mPath = path;
    loadingStuff->mJob.mPath = path;
    JobQueuePush( &loadingStuff->mJob );

    return nullptr;
  }

}
