#pragma once

//#include "tac-std-lib/string/tac_string.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_span.h"

//#include "tac-dx/dx12/tac_dx12_root_sig_bindings.h"

#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_bind_cache.h"

//#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"

#include <d3d12.h> // ID3D12PipelineState
//#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

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
    struct BindlessArray
    {
      void SetBufferAtIndex( int, BufferHandle );
      void SetTextureAtIndex( int, TextureHandle );
      void SetSamplerAtIndex( int, SamplerHandle );

    private:
      void SetArrayElement( int, int );
      D3D12ProgramBindType mType          {};
      Vector< int >        mHandleIndexes {};
    };

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
      Vector< int >             mHandleIndexes {};
      D3D12ProgramBindDesc       mBinding       {};
    };

    struct Variables
    {
      Variables() = default;
      Variables( const D3D12ProgramBindDescs& );

      int             size() const;
      const Variable* begin() const;
      const Variable* end() const;
      Variable*       begin();
      Variable*       end();
      const Variable* data() const;
      Variable*       data();
      const Variable& operator[]( int ) const;

    private:

      Vector< Variable > mShaderVariables;
    };

    bool IsValid() const;

    PCom< ID3D12PipelineState > mPSO;
    PCom< ID3D12RootSignature > mRootSignature;
    Variables                   mShaderVariables;
    PipelineParams              mPipelineParams;
    PipelineBindCache           mPipelineBindCache;
    bool                        mIsCompute{};
  };
} // namespace Tac::Render

