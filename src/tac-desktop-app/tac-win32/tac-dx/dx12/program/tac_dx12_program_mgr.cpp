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

  struct PreprocessorInput : public AssetPathStrings
  {
    PreprocessorInput( const ProgramParams& programParams )
    {
      const IDevice* device{ Render::RenderApi::GetRenderDevice() };
      const IDevice::Info info{ device->GetInfo() };
      const ProgramAttribs programAttribs{ info.mProgramAttribs };
      const StringView shaderDir{ programAttribs.mDir };
      const StringView shaderExt{ programAttribs.mExt };

      Vector< AssetPathString > assetPaths;
      for( const String& input : programParams.mInputs )
      {
        const AssetPathString inputAsset{ shaderDir + input + shaderExt };
        push_back( inputAsset );
      }
    }
  };

  static DXCCompileOutput Compile( ProgramParams programParams, Errors& errors )
  {
    TAC_RAISE_ERROR_IF_RETURN( {}, programParams.mInputs.empty(), "Missing shader sources" );

    if( programParams.mName.empty() && programParams.mInputs.size() == 1 )
      programParams.mName = programParams.mInputs[ 0 ];

    TAC_RAISE_ERROR_IF_RETURN( {}, programParams.mName.empty(), "Missing shader name" );


#if 0
    if( programParams.mInputs.size() > 1 )
    {
      String combinedInputs;
      for( const String& input : programParams.mInputs )
      {
        TAC_CALL_RET( {}, const String inputStr{
          FileSys::LoadFilePath( sShaderDir + input + sShaderExt, errors ) } );

        combinedInputs += inputStr;
        combinedInputs += "\n";
      }

      TAC_CALL_RET( {}, FileSys::SaveToFile( filePath, combinedInputs, errors ) );
    }
#endif

    const IDevice* device{ Render::RenderApi::GetRenderDevice() };
    const IDevice::Info info{ device->GetInfo() };
    const ProgramAttribs programAttribs{ info.mProgramAttribs };
    const StringView shaderExt{ programAttribs.mExt };

    const PreprocessorInput assetPaths( programParams );

    TAC_CALL_RET( {}, const String preprocessedShader{
      HLSLPreprocessor::Process( assetPaths, errors ) } );

    const FileSys::Path outputDir{ RenderApi::GetShaderOutputPath() };
    const String fileName{ programParams.mName + shaderExt };
    const DXCCompileParams input
    {
      .mFileName           { fileName },
      .mPreprocessedShader { preprocessedShader },
      .mShaderModel        { sShaderModel },
      .mOutputDir          { outputDir },
    };

    return DXCCompile( input, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ProgramMgr::Init( Params params, Errors& errors )
  {
    mDevice = params.mDevice;
    TAC_ASSERT( mDevice );

    mPipelineMgr = params.mPipelineMgr;
    TAC_ASSERT( mPipelineMgr );

    const D3D_SHADER_MODEL highestShaderModel{ GetHighestShaderModel( mDevice ) };
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

  void          DX12ProgramMgr::CreateProgramAtIndex( const ProgramHandle h ,
                                                      dynmc ProgramParams params,
                                                      dynmc Errors& errors )
  {
    if( params.mName.empty() && params.mInputs.size() == 1 )
      params.mName = params.mInputs[ 0 ];

    TAC_RAISE_ERROR_IF( params.mInputs.empty(), "Missing shader sources" );
    TAC_RAISE_ERROR_IF( params.mName.empty(), "Missing shader name" );

    // Basically const, but 
    TAC_CALL( const DXCCompileOutput output{ Compile( params, errors ) } );

    const D3D12ProgramBindDescs programBindDescs( output.mReflInfo.mReflBindings );

    const DX12Program::Inputs programInputs( output.mReflInfo.mInputs );

    TAC_CALL( const DX12Program::HotReloadInputs hotReloadInputs( params, errors ) );

    mPrograms[ h.GetIndex() ] = DX12Program
    {
      .mVSBlob           { output.mVSBlob },
      .mVSBytecode       { output.GetVSBytecode() },
      .mPSBlob           { output.mPSBlob },
      .mPSBytecode       { output.GetPSBytecode() },
      .mCSBlob           { output.mCSBlob },
      .mCSBytecode       { output.GetCSBytecode() },
      .mProgramBindDescs { programBindDescs },
      .mProgramParams    { params },
      .mInputs           { programInputs },
      .mHotReloadInputs  { hotReloadInputs },
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
    const DX12Program* program{ FindProgram( h ) };
    if( !program )
      return {};

    String result;
    for( const D3D12ProgramBindDesc& binding : program->mProgramBindDescs )
    {
      result += binding.mName;
      result += " ";
    }

    return result;
  }

  void          DX12ProgramMgr::HotReload( Errors& errors )
  {
    const Timestamp curTime{ Timestep::GetElapsedTime() };
    const TimestampDifference diffTime{ curTime - sHotReloadTick };
    if( diffTime.mSeconds < 1.0f )
      return;

    Vector< ProgramHandle > reloadedPrograms;

    const int n{ mPrograms.size()};
    for( int i{}; i < n; ++i )
    {
      const ProgramHandle h{ i };
      dynmc DX12Program& program{ mPrograms[ i ] };

      int updatedTimeCount{};
      for( dynmc DX12Program::HotReloadInput& hotReloadInput : program.mHotReloadInputs )
      {
        TAC_CALL( const FileSys::Time fileTime{
          FileSys::GetFileLastModifiedTime( hotReloadInput.mFilePath, errors ) } );

        if( hotReloadInput.mFileTime != fileTime )
        {
          hotReloadInput.mFileTime = fileTime;
          ++updatedTimeCount;
        }
      }

      if( !updatedTimeCount )
        continue;

      Errors reloadErrors;

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

      reloadedPrograms.push_back( h );
    }

    sHotReloadTick = curTime;

    const Span< ProgramHandle > reloadedProgramSpan( reloadedPrograms.data(),
                                                     reloadedPrograms.size() );
    TAC_CALL( mPipelineMgr->HotReload( reloadedProgramSpan, errors ) );
  }

} // namespace Tac::Render

