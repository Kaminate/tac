#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_program.h" // self-inc

#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_compiler.h" // CompileShaderFromString
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_postprocess.h"
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h"
#include "src/shell/windows/renderer/dx11/tac_dx11_namer.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/system/tac_os.h"

#include <d3dcompiler.h> // D3D_BLOB_INPUT_SIGNATURE_BLOB

namespace Tac::Render
{
  static const char* GetEntryPoint( const ShaderType type )
    {
      switch( type )
      {
      case ShaderType::Vertex: return "VS";
      case ShaderType::Fragment: return "PS";
      case ShaderType::Geometry: return "GS";
      case ShaderType::Compute: return "CS";
      default: return nullptr;
      }
    }

    static const char* GetShaderModel( const ShaderType type )
    {
      // Although shader model 5.1 is the highest shader model supported by fxc.exe,
      // When using it, I get D3D11 ERROR : ID3D11Device::CreateVertexShader 
      // Shader must be vs_4_0, vs_4_1, or vs_5_0.

      switch( type )
      {
      case ShaderType::Vertex: return "vs_5_0";
      case ShaderType::Fragment: return "ps_5_0";
      case ShaderType::Geometry: return "gs_5_0";
      case ShaderType::Compute: return "cs_5_0";
      default: return nullptr;
      }
    }


  struct DX11ProgramLoader
  {
    DX11ProgramLoader( const ShaderNameStringView& shaderName, Errors& errors )
      : mShaderName{ shaderName }
      , mDevice{ ( ID3D11Device* )RendererDirectX11::GetInstance()->mDevice }
    {
      TAC_ASSERT( !shaderName.empty() );
      const AssetPathStringView assetPath = GetShaderAssetPath( mShaderName );
      mShaderStringOrig = TAC_CALL( LoadAssetPath( assetPath, errors ) );
      mShaderStringFull = TAC_CALL( DX11PreprocessShader( assetPath, errors ) );
      mProgram.mConstantBuffers = PostprocessShaderSource( mShaderStringFull );
      TAC_ASSERT( !mProgram.mConstantBuffers.empty() );

      TAC_CALL( TryLoadVertexShader( errors ) );
      TAC_CALL( TryLoadPixelShader( errors ) );
      TAC_CALL( TryLoadGeometryShader( errors ) );
    }

    bool HasShader(const ShaderType type)
    {
      const char* entryPoint = GetEntryPoint( type );
      if( !entryPoint )
        return false;

      const auto search = ShortFixedString::Concat(entryPoint, "(");
      const bool result = mShaderStringFull.contains((StringView)search);
      return result;
    }

    PCom<ID3DBlob> CompileShaderBlob( const ShaderType type, Errors& errors )
    {
      const char* shaderModel = GetShaderModel( type );
      if( !shaderModel )
        return {};

      const char* entryPoint = GetEntryPoint( type );
      if( !entryPoint )
        return {};

      return CompileShaderFromString( mShaderName,
                                      mShaderStringOrig,
                                      mShaderStringFull,
                                      entryPoint,
                                      shaderModel,
                                      errors );
    }

    void TryLoadVertexShader( Errors& errors )
    {
      if( !HasShader( ShaderType::Vertex ) )
        return;

      PCom<ID3DBlob> blob = TAC_CALL( CompileShaderBlob( ShaderType::Vertex, errors ) );
      TAC_DX11_CALL( mDevice->CreateVertexShader(
                            blob->GetBufferPointer(),
                            blob->GetBufferSize(),
                            nullptr,
                            &mProgram.mVertexShader ) );
      TAC_DX11_CALL( D3DGetBlobPart(
                            blob->GetBufferPointer(),
                            blob->GetBufferSize(),
                            D3D_BLOB_INPUT_SIGNATURE_BLOB,
                            0,
                            &mProgram.mInputSig) );
      SetDebugName( mProgram.mVertexShader, mShaderName );
    }

    void TryLoadPixelShader( Errors& errors )
    {
      if( !HasShader( ShaderType::Fragment ) )
        return;

      PCom<ID3DBlob> blob = TAC_CALL( CompileShaderBlob( ShaderType::Fragment , errors ) );
      TAC_DX11_CALL( mDevice->CreatePixelShader(
                     blob->GetBufferPointer(),
                     blob->GetBufferSize(),
                     nullptr,
                     &mProgram.mPixelShader ) );

      SetDebugName( mProgram.mPixelShader, mShaderName );
    }

    void TryLoadGeometryShader( Errors& errors )
    {
      if( !HasShader( ShaderType::Geometry ) )
        return;

      PCom<ID3DBlob> blob = TAC_CALL( CompileShaderBlob( ShaderType::Geometry , errors ) );
      TAC_DX11_CALL( mDevice->CreateGeometryShader(
                     blob->GetBufferPointer(),
                     blob->GetBufferSize(),
                     nullptr,
                     &mProgram.mGeometryShader ) );
      SetDebugName( mProgram.mGeometryShader, mShaderName );
    }


    ShaderNameStringView   mShaderName;
    String                 mShaderStringOrig;
    String                 mShaderStringFull;
    ID3D11Device*          mDevice;
    DX11Program            mProgram;
  };


  DX11Program DX11LoadProgram( const ShaderNameStringView& shaderName, Errors& errors )
  {
    for( ;; )
    {
      const DX11ProgramLoader loader( shaderName, errors );

      if( errors && IsDebugMode )
      {
        const auto dbgMsg = String() + "Error compiling shader: " + shaderName;
        OS::OSDebugPrintLine( dbgMsg );
        OS::OSDebugPopupBox( dbgMsg );
        OS::OSDebugBreak();
        errors.clear();
        continue;
      }

      return loader.mProgram;
    }
  }
} // namespace Tac::Render



