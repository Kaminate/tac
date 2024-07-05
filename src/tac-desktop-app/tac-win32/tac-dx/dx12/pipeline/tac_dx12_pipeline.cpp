#include "tac_dx12_pipeline.h" // self-inc
#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------


  bool DX12Pipeline:: IsValid() const
  {
    return mPSO.Get() != nullptr;
  }

  DX12Pipeline::Variable::Variable( D3D12ProgramBinding binding )
  {
    mBinding = binding;
    mHandleIndexes.resize( binding.mBindCount, -1 );
  }

  void DX12Pipeline::Variable::SetBuffer( BufferHandle h )
  {
    TAC_ASSERT( mBinding.IsBuffer() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTexture( TextureHandle h )
  {
    TAC_ASSERT( mBinding.IsTexture() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetBufferAtIndex( int i, BufferHandle h )
  {
    TAC_ASSERT( mBinding.IsBuffer() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTextureAtIndex( int i, TextureHandle h )
  {
    TAC_ASSERT( mBinding.IsTexture() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetSamplerAtIndex( int i, SamplerHandle h )
  {
    TAC_ASSERT( mBinding.IsSampler() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetSampler( SamplerHandle h )
  {
    TAC_ASSERT( mBinding.IsSampler() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetElement( int iHandle )
  {
    TAC_ASSERT( mHandleIndexes.size() == 1 );
    TAC_ASSERT( mBinding.mBindCount == 1 );
    mHandleIndexes[ 0 ] = iHandle;
  }

  void DX12Pipeline::Variable::SetArrayElement( int iArray, int iHandle )
  {
    TAC_ASSERT_INDEX( iArray, 1000 ); // sanity
    TAC_ASSERT( mBinding.mBindCount >= 0 ); // sanity

    // resize unbounded array
    if( !mBinding.mBindCount && iArray >= mHandleIndexes.size() )
      mHandleIndexes.resize( iArray + 1, -1 );

    mHandleIndexes[ iArray ] = iHandle;
  }


  StringView DX12Pipeline::Variable::GetName() const
  {
    return mBinding.mName;
  }

  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper,
    DX12TextureMgr* mTextureMgr,
    DX12SamplerMgr* mSamplerMgr,
    DX12BufferMgr* mBufferMgr ) const
  {
    const int n{ mHandleIndexes.size() };
    DX12Descriptor* dst{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * n ) };

    Span< DX12Descriptor > result( dst, n );

    for( int iHandle : mHandleIndexes )
    {
      DX12Descriptor descriptor{
        GetDescriptor( iHandle, transitionHelper, mTextureMgr, mSamplerMgr, mBufferMgr ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dst++ = descriptor;
    }

    return result;
  }

  DX12Descriptor DX12Pipeline::Variable::GetDescriptor( int iHandle,
                                                        DX12TransitionHelper* transitionHelper,
                                                        DX12TextureMgr* textureMgr,
                                                        DX12SamplerMgr* samplerMgr,
                                                        DX12BufferMgr* bufferMgr ) const
  {
    const D3D12ProgramBinding binding{ mBinding };

    if( binding.IsTexture() )
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );

      textureMgr->TransitionTexture( textureHandle, transitionHelper );

      TAC_ASSERT( texture->mState & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );

      if( binding.mType == D3D12ProgramBinding::kTextureSRV )
      {
        return texture->mSRV.GetValue();
      }

      if( binding.mType == D3D12ProgramBinding::kTextureUAV )
      {
        TAC_ASSERT( texture->mUAV.HasValue() );
        TAC_ASSERT( texture->mState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
        return texture->mUAV.GetValue();
      }
    }

    if( binding.IsBuffer() ) // this includes constant buffers
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );

      TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );

      if( binding.mType == D3D12ProgramBinding::kBufferSRV )
      {
        return buffer->mSRV.GetValue();
      }

      if( binding.mType == D3D12ProgramBinding::kBufferUAV )
      {
        TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
        return buffer->mUAV.GetValue();
      }

      if( binding.mType == D3D12ProgramBinding::kConstantBuffer )
      {
        TAC_ASSERT( buffer->mState & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
        TAC_ASSERT_UNIMPLEMENTED;
      }
    }

    if( binding.IsSampler() )
    {
      DX12Sampler* sampler{ samplerMgr->FindSampler( SamplerHandle{ iHandle } ) };
      TAC_ASSERT( sampler );

      return sampler->mDescriptor;
    }

    return {};
  }
} // namespace Tac::Render

