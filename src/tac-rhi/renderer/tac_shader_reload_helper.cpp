#include "tac_shader_reload_helper.h" // self-inc

#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/shell/tac_shell_timestep.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/assetmanagers/tac_asset.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  struct ShaderReloadInfo
  {
    ShaderNameString mShaderName;
    Filesystem::Path mFullPath;
    Filesystem::Time mFileModifyTime{};
  };

  // -----------------------------------------------------------------------------------------------

  static ShaderReloadInfo sShaderReloadInfos[ kMaxPrograms ];

  // -----------------------------------------------------------------------------------------------



  void               ShaderReloadHelperAdd( const ShaderHandle shaderHandle,
                                            const ShaderNameStringView& shaderName,
                                            Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    const AssetPathStringView shaderAssetPath = GetShaderAssetPath(shaderName);
    const Filesystem::Path fullPath( shaderAssetPath );

    const Filesystem::Time time =
      TAC_CALL( Filesystem::GetFileLastModifiedTime( fullPath, errors ) );

    TAC_ASSERT( time.IsValid() );

    sShaderReloadInfos[ ( int )shaderHandle ] = ShaderReloadInfo
    {
      .mShaderName = shaderName,
      .mFullPath = fullPath,
      .mFileModifyTime = time,
    };
  }

  void               ShaderReloadHelperRemove( const ShaderHandle shaderHandle )
  {
    if constexpr( IsDebugMode )
      sShaderReloadInfos[ ( int )shaderHandle ] = ShaderReloadInfo();
  }

  static void        ShaderReloadHelperUpdateAux( ShaderReloadInfo* shaderReloadInfo,
                                                  ShaderReloadFunction* shaderReloadFunction,
                                                  Errors& errors)
  {
    TAC_ASSERT( shaderReloadInfo->mFileModifyTime.IsValid() );

    const Filesystem::Time time = TAC_CALL(
      Filesystem::GetFileLastModifiedTime( shaderReloadInfo->mFullPath, errors ) );

    if( time == shaderReloadInfo->mFileModifyTime )
      return;

    const ShaderHandle shaderHandle( ( int )( shaderReloadInfo - sShaderReloadInfos ) );
    shaderReloadInfo->mFileModifyTime = time;
    shaderReloadFunction( shaderHandle, shaderReloadInfo->mShaderName, errors );
  }

  void               ShaderReloadHelperUpdate( ShaderReloadFunction* shaderReloadFunction,
                                               Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    static Timestamp lastUpdateSeconds;

    const Timestamp curSec = Timestep::GetElapsedTime();
    const TimestampDifference shaderReloadPeriodSecs = 0.5f;
    if( curSec < lastUpdateSeconds + shaderReloadPeriodSecs )
      return;

    lastUpdateSeconds = curSec;
    for( ShaderReloadInfo& shaderReloadInfo : sShaderReloadInfos )
      ShaderReloadHelperUpdateAux( &shaderReloadInfo, shaderReloadFunction, errors );
  }


} // namespace Tac::Render

