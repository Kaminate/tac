#include "tac_dx12_program.h"

namespace Tac::Render
{
  DX12Program::HotReloadInputs::HotReloadInputs( const ProgramParams& params, Errors& errors )
  {
    const IDevice* device{ Render::RenderApi::GetRenderDevice() };
    const IDevice::Info info{ device->GetInfo() };
    const StringView shaderDir{ info.mProgramAttribs.mDir };
    const StringView shaderExt{ info.mProgramAttribs.mExt };

    for( const String& input : params.mInputs )
    {
      const FileSys::Path filePath{ shaderDir + input + shaderExt };
      TAC_CALL( const FileSys::Time fileTime{
        FileSys::GetFileLastModifiedTime( filePath, errors ) } );

      const DX12Program::HotReloadInput hotReloadInput
      {
        .mFilePath{ filePath },
        .mFileTime{ fileTime },
      };
      push_back( hotReloadInput );
    }
  }

  DX12Program::Inputs::Inputs( const DXCReflInfo::Inputs& inputs )
  {
    const int n{ inputs.size() };
    reserve( n );
    for( int i{}; i < n; ++i )
    {
      const DXCReflInfo::Input& reflInput{ inputs[ i ] };
      emplace_back() = DX12Program::Input
      {
        .mName     { reflInput.mName },
        .mIndex    { reflInput.mIndex },
        .mRegister { reflInput.mRegister },
      };
    }
  }

} // namespace Tac::Render

