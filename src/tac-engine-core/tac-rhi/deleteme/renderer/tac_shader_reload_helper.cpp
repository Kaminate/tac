#include "tac_shader_reload_helper.h" // self-inc

#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/tac_render_frame.h"

#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"

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

    const AssetPathStringView shaderAssetPath { GetShaderAssetPath( shaderName ) };
    const Filesystem::Path fullPath( shaderAssetPath );

    TAC_CALL( const Filesystem::Time time{
       Filesystem::GetFileLastModifiedTime( fullPath, errors ) } );

    TAC_ASSERT( time.IsValid() );

    sShaderReloadInfos[ ( int )shaderHandle ] = ShaderReloadInfo
    {
      .mShaderName { shaderName },
      .mFullPath { fullPath },
      .mFileModifyTime { time },
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

    TAC_CALL( const Filesystem::Time time{
      Filesystem::GetFileLastModifiedTime( shaderReloadInfo->mFullPath, errors ) } );

    if( time == shaderReloadInfo->mFileModifyTime )
      return;

    const ShaderHandle shaderHandle( ( int )( shaderReloadInfo - sShaderReloadInfos ) );
    shaderReloadInfo->mFileModifyTime = time;
    shaderReloadFunction( shaderHandle, shaderReloadInfo->mShaderName, errors );
  }

  void               ShaderReloadHelperUpdate( float dt,
                                               ShaderReloadFunction* shaderReloadFunction,
                                               Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    static float accumSec;
    accumSec += dt;
    if( accumSec < 0.5f )
      return;

    accumSec = 0;

    for( ShaderReloadInfo& shaderReloadInfo : sShaderReloadInfos )
      ShaderReloadHelperUpdateAux( &shaderReloadInfo, shaderReloadFunction, errors );
  }


} // namespace Tac::Render

