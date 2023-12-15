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
      case ShaderType::Vertex: return  "VS";
      case ShaderType::Fragment: return  "PS";
      case ShaderType::Geometry: return  "GS";
      default: return nullptr;
      }
    }

    static const char* GetShaderModel( const ShaderType type )
    {
      switch( type )
      {
      case ShaderType::Vertex: return  "vs_4_0";
      case ShaderType::Fragment: return  "ps_4_0";
      case ShaderType::Geometry: return  "gs_4_0";
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
      mShaderStringOrig = TAC_CALL( LoadAssetPath, assetPath, errors );
      mShaderStringFull = TAC_CALL( PreprocessShaderSource, mShaderStringOrig, errors );
      mProgram.mConstantBuffers = PostprocessShaderSource( mShaderStringFull );
      TAC_ASSERT( !mProgram.mConstantBuffers.empty() );

      for( int i = 0; i < ( int )ShaderType::Count; ++i )
      {
        mBlobs[ i ] = TAC_CALL( CompileShaderBlob, ( ShaderType )i, errors )
      }

      TAC_CALL( TryLoadVertexShader, errors );
      TAC_CALL( TryLoadPixelShader, errors );
      TAC_CALL( TryLoadGeometryShader, errors );
    }

    ~DX11ProgramLoader()
    {
      for( ID3DBlob* blob : mBlobs )
        if( blob )
          blob->Release();
    }


    ID3DBlob* CompileShaderBlob( const ShaderType type, Errors& errors )
    {
      const char* shaderModel = GetShaderModel( type );
      if( !shaderModel )
        return nullptr;

      const char* entryPoint = GetEntryPoint( type );
      if( !entryPoint )
        return nullptr;


      const auto search = ShortFixedString::Concat(entryPoint, "(");
      const bool hasEntryPoint = mShaderStringFull.contains((StringView)search);
      if( !hasEntryPoint )
        return nullptr;

      return CompileShaderFromString( mShaderName,
                                      mShaderStringOrig,
                                      mShaderStringFull,
                                      entryPoint,
                                      shaderModel,
                                      errors );
    }

    void TryLoadVertexShader( Errors& errors )
    {
      ID3DBlob* pVSBlob = GetBlob( ShaderType::Vertex );
      if( !pVSBlob )
        return;

      TAC_DX11_CALL( mDevice->CreateVertexShader,
                            pVSBlob->GetBufferPointer(),
                            pVSBlob->GetBufferSize(),
                            nullptr,
                            &mProgram.mVertexShader );
      TAC_DX11_CALL( D3DGetBlobPart,
                            pVSBlob->GetBufferPointer(),
                            pVSBlob->GetBufferSize(),
                            D3D_BLOB_INPUT_SIGNATURE_BLOB,
                            0,
                            &mProgram.mInputSig);
      SetDebugName( mProgram.mVertexShader, mShaderName );
    }

    void TryLoadPixelShader( Errors& errors )
    {
      ID3DBlob* pPSBlob = GetBlob( ShaderType::Fragment );
      if( !pPSBlob )
        return;

      TAC_DX11_CALL( mDevice->CreatePixelShader,
                     pPSBlob->GetBufferPointer(),
                     pPSBlob->GetBufferSize(),
                     nullptr,
                     &mProgram.mPixelShader );

      SetDebugName( mProgram.mPixelShader, mShaderName );
    }

    void TryLoadGeometryShader( Errors& errors )
    {
      ID3DBlob* blob = GetBlob( ShaderType::Geometry );
      if( !blob )
        return;

      TAC_DX11_CALL( mDevice->CreateGeometryShader,
                     blob->GetBufferPointer(),
                     blob->GetBufferSize(),
                     nullptr,
                     &mProgram.mGeometryShader );
      SetDebugName( mProgram.mGeometryShader, mShaderName );
    }

    ID3DBlob* GetBlob( ShaderType type ) { return mBlobs[ ( int )type ]; }

    ShaderNameStringView   mShaderName;
    StringView             mShaderStringOrig;
    StringView             mShaderStringFull;

    ID3DBlob*              mBlobs[ ( int )ShaderType::Count ]{};
    ID3D11Device*          mDevice;

    DX11Program                mProgram;
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



