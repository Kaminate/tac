#include "tac_dx12_program_mgr.h" // self-inc

#include "tac-dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-dx/dxc/tac_dxc.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-rhi/render3/tac_render_backend.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  static const D3D_SHADER_MODEL sShaderModel { D3D_SHADER_MODEL_6_5 };
  static Timestamp              sHotReloadTick;
  static const char*            sShaderExt{ ".hlsl" };
  static const char*            sShaderDir{ "assets/hlsl/" };

  static D3D12_SHADER_BYTECODE IDxcBlobToBytecode( PCom<IDxcBlob>& blob )
  {
    return blob ?
      D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }

  static D3D_SHADER_MODEL GetHighestShaderModel( ID3D12Device* device )
  {
    const D3D_SHADER_MODEL lowestDefined { D3D_SHADER_MODEL_5_1 };
    const D3D_SHADER_MODEL highestDefined { D3D_SHADER_MODEL_6_7 }; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel { highestDefined };
         shaderModel >= lowestDefined;
         shaderModel = D3D_SHADER_MODEL( shaderModel - 1 ) )
    {
      // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model
      //   After the function completes successfully, the HighestShaderModel field contains the
      //   highest shader model that is both supported by the device and no higher than the
      //   shader model passed in.
      dynmc D3D12_FEATURE_DATA_SHADER_MODEL featureData{ shaderModel };
      if( SUCCEEDED( device->CheckFeatureSupport(
        D3D12_FEATURE_SHADER_MODEL,
        &featureData,
        sizeof( D3D12_FEATURE_DATA_SHADER_MODEL ) ) ) )

        // For some godforsaken fucking reason, this isn't the same as shaderModel
        return featureData.HighestShaderModel;
    }

    return lowestDefined;
  }

  static DXCCompileOutput Compile( StringView fileStem, Errors& errors )
  {
    TAC_RAISE_ERROR_IF_RETURN( {}, fileStem.empty(), "No shader specified to compile" );

    const String fileName{ fileStem + sShaderExt };
    const String filePath{ sShaderDir + fileName };

    TAC_CALL_RET( {}, const String preprocessedShader{ HLSLPreprocess( filePath, errors ) } );

    const FileSys::Path outputDir{ RenderApi::GetShaderOutputPath() };
    const DXCCompileParams input
    {
      .mFileName           { fileName },
      .mPreprocessedShader { preprocessedShader },
      .mShaderModel        { sShaderModel },
      .mOutputDir          { outputDir },
    };

    TAC_CALL_RET( {}, DXCCompileOutput output{ DXCCompile( input, errors ) } );
    return output;
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ProgramMgr::Init( Params params, Errors& errors )
  {
    mDevice = params.mDevice;
    TAC_ASSERT(mDevice);
    
    mPipelineMgr = params.mPipelineMgr;
    TAC_ASSERT(mPipelineMgr);

    const D3D_SHADER_MODEL highestShaderModel { GetHighestShaderModel( mDevice ) };
    TAC_RAISE_ERROR_IF( sShaderModel > highestShaderModel, "Shader model too high" );
  }

  DX12Program*  DX12ProgramMgr::FindProgram( ProgramHandle h )
  {
    return h.IsValid() ? &mPrograms[ h.GetIndex() ] : nullptr;
  }

  void          DX12ProgramMgr::DestroyProgram( ProgramHandle h )
  {
    if( h.IsValid() )
    {
      FreeHandle( h );
      mPrograms[ h.GetIndex() ] = {};
    }
  }

  void          DX12ProgramMgr::CreateProgramAtIndex( ProgramHandle h ,
                                                      ProgramParams params,
                                                      Errors& errors )
  {
    TAC_CALL( DXCCompileOutput output{ Compile( params.mFileStem, errors ) } );

    const D3D12ProgramBindings bindings( output.mReflInfo.mReflBindings.data(),
                                         output.mReflInfo.mReflBindings.size() );

    const int programInputCount{ output.mReflInfo.mInputs.size() };
    Vector< DX12Program::Input > programInputs( programInputCount );
    for( int i{}; i < programInputCount; ++i )
    {
      const DXCReflInfo::Input& reflInput{ output.mReflInfo.mInputs[ i ] };
      programInputs[ i ] = DX12Program::Input
      {
        .mName     { reflInput.mName },
        .mIndex    { reflInput.mIndex },
        .mRegister { reflInput.mRegister },
      };
    }

    const String filePath{ sShaderDir + params.mFileStem + sShaderExt };
    TAC_CALL( FileSys::Time fileTime{
      FileSys::GetFileLastModifiedTime( filePath, errors ) } );

    mPrograms[ h.GetIndex() ] = DX12Program
    {
      .mFileStem        { params.mFileStem },
      .mFileTime        { fileTime },
      .mVSBlob          { output.mVSBlob },
      .mVSBytecode      { IDxcBlobToBytecode( output.mVSBlob ) },
      .mPSBlob          { output.mPSBlob },
      .mPSBytecode      { IDxcBlobToBytecode( output.mPSBlob ) },
      .mProgramBindings { bindings },
      .mProgramParams   { params },
      .mInputs          { programInputs },
    };

  }

  ProgramHandle DX12ProgramMgr::CreateProgram( ProgramParams params,
                                               Errors& errors )
  {
    const ProgramHandle h{ AllocProgramHandle() };
    CreateProgramAtIndex( h, params, errors );
    return h;
  }

  String        DX12ProgramMgr::GetProgramBindings_TEST( ProgramHandle h )
  {
    DX12Program* program{ FindProgram( h ) };
    if( !program )
      return {};

    String result;
    for( const D3D12ProgramBinding& binding : program->mProgramBindings )
    {
      result += binding.mName;
      result += " ";
    }

    return result;
  }

  void          DX12ProgramMgr::HotReload( Errors& errors )
  {
    Timestamp curTime{ Timestep::GetElapsedTime() };
    TimestampDifference diffTime{ curTime - sHotReloadTick };
    if( diffTime.mSeconds < 1.0f )
      return;

    Vector< ProgramHandle > reloadedPrograms;

    const int n{ mPrograms.size()};
    for( int i{}; i < n; ++i )
    {
      ProgramHandle h{i};
      DX12Program& program{ mPrograms[ i ] };
      if( program.mFileStem.empty() )
        continue;

      const String filePath{ sShaderDir + program.mFileStem + sShaderExt };
      TAC_CALL( FileSys::Time fileTime{ FileSys::GetFileLastModifiedTime( filePath, errors ) } );

      if( fileTime == program.mFileTime )
        continue;

      Errors reloadErrors;

#if 0
      DXCCompileOutput output;
      for( ;; )
      {
        output = Compile( program.mFileStem, errors );
        if( reloadErrors )
        {
          const String errorStr{ reloadErrors.ToString() };
          OS::OSDebugPrintLine( errorStr );
          OS::OSDebugPopupBox( errorStr );
          reloadErrors = {};
        }
        else
        {
          break;
        }
      }

      program.mFileTime   = fileTime;
      program.mVSBlob     = output.mVSBlob;
      program.mPSBlob     = output.mPSBlob;
      program.mVSBytecode = IDxcBlobToBytecode( output.mVSBlob );
      program.mPSBytecode = IDxcBlobToBytecode( output.mPSBlob );
#else

      for( ;; )
      {
        CreateProgramAtIndex( h, program.mProgramParams, reloadErrors );
        if( reloadErrors )
        {
          const String errorStr{ reloadErrors.ToString() };
          OS::OSDebugPrintLine( errorStr );
          OS::OSDebugPopupBox( errorStr );
          reloadErrors = {};
        }
        else
        {
          break;
        }
      }
#endif

      reloadedPrograms.push_back( h );
    }

    sHotReloadTick = curTime;

    Span< ProgramHandle > reloadedProgramSpan{
      reloadedPrograms.data(),
      reloadedPrograms.size() };

    TAC_CALL( mPipelineMgr->HotReload( reloadedProgramSpan, errors ) );
  }

} // namespace Tac::Render

