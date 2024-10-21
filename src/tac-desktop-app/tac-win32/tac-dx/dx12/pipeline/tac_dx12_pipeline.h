#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_span.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_bind_cache.h"

#include <d3d12.h> // ID3D12PipelineState

namespace Tac { struct Errors; }
namespace Tac::Render
{
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
      Variable( D3D12ProgramBindDesc );

      void SetBuffer( BufferHandle ) override;
      void SetTexture( TextureHandle ) override;
      void SetSampler( SamplerHandle ) override;
      void SetBufferAtIndex( int, BufferHandle ) override;
      void SetTextureAtIndex( int, TextureHandle ) override;
      void SetSamplerAtIndex( int, SamplerHandle ) override;

      void SetBindlessArray( IShaderBindlessArray* ) override;

      StringView             GetName() const;
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
      Vector< int >        mHandleIndexes {};
      D3D12ProgramBindDesc mBinding       {};
    };

    struct Variables : public Vector< Variable >
    {
      Variables() = default;
      Variables( const D3D12ProgramBindDescs& );
    };

    bool IsValid() const;

    PCom< ID3D12PipelineState > mPSO               {};
    PCom< ID3D12RootSignature > mRootSignature     {};
    Variables                   mShaderVariables   {};
    PipelineParams              mPipelineParams    {};
    PipelineBindCache           mPipelineBindCache {};
    bool                        mIsCompute         {};
  };
} // namespace Tac::Render

