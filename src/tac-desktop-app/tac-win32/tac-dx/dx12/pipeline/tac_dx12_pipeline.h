#pragma once

//#include "tac-std-lib/string/tac_string.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_span.h"

//#include "tac-dx/dx12/tac_dx12_root_sig_bindings.h"


//#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"

#include <d3d12.h> // ID3D12PipelineState
//#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct D3D12ProgramBinding;
  struct DX12Descriptor;
  struct DX12TextureMgr;
  struct DX12SamplerMgr;
  struct DX12BufferMgr;
  struct DX12TransitionHelper;
}

namespace Tac::Render
{
  struct DX12Pipeline
  {
    struct Variable : public IShaderVar
    {
      Variable() = default;
      Variable( const D3D12ProgramBinding* );

      void SetBuffer( BufferHandle ) override;
      void SetTexture( TextureHandle ) override;
      void SetSampler( SamplerHandle ) override;
      void SetBufferAtIndex( int, BufferHandle ) override;
      void SetTextureAtIndex( int, TextureHandle ) override;
      void SetSamplerAtIndex( int, SamplerHandle ) override;
      StringView GetName() const;


      Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper*,
                                             DX12TextureMgr*,
                                             DX12SamplerMgr*,
                                             DX12BufferMgr* ) const;

    private:
      void SetArrayElement( int, int );
      void SetElement( int );
      DX12Descriptor GetDescriptor( int,
                                    DX12TransitionHelper*,
                                    DX12TextureMgr*,
                                    DX12SamplerMgr*,
                                    DX12BufferMgr* ) const;

    public:
      Vector< int >              mHandleIndexes;
      const D3D12ProgramBinding* mBinding{};
    };

    PCom< ID3D12PipelineState > mPSO;
    PCom< ID3D12RootSignature > mRootSignature;
    Vector< Variable >          mShaderVariables;
  };
} // namespace Tac::Render

